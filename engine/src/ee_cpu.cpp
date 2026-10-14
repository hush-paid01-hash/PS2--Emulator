#include "retroshelf/ee_cpu.hpp"

#include <iomanip>
#include <sstream>

namespace retroshelf::ee {

EeCpu::EeCpu() { reset(); }
void EeCpu::reset() {
    gpr_.fill({});
    pc_ = 0;
    branch_pending_ = false;
    branch_target_ = 0;
    hi_ = 0;
    lo_ = 0;
    cp0_.fill(0);
}
Register128 EeCpu::gpr(std::size_t index) const {
    return index < GprCount ? gpr_[index] : Register128{};
}
void EeCpu::set_gpr(std::size_t index, Register128 value) {
    if (index > 0 && index < GprCount) gpr_[index] = value;
    gpr_[0] = {};
}
std::uint64_t EeCpu::pc() const { return pc_; }
void EeCpu::set_pc(std::uint64_t value) { pc_ = value; }
bool EeCpu::branch_pending() const { return branch_pending_; }
std::uint64_t EeCpu::branch_target() const { return branch_target_; }
std::uint64_t EeCpu::hi() const { return hi_; }
std::uint64_t EeCpu::lo() const { return lo_; }
std::uint32_t EeCpu::cp0(std::size_t index) const {
    return index < cp0_.size() ? cp0_[index] : 0;
}

bool EeCpu::step(std::uint32_t instruction, std::string* error,
                 const ReadWord& read_word, const WriteWord& write_word) {
    const std::uint64_t current_pc = pc_;
    const bool applying_previous_branch = branch_pending_;
    const std::uint64_t previous_target = branch_target_;
    branch_pending_ = false;

    const auto opcode = (instruction >> 26U) & 0x3FU;
    const auto rs = (instruction >> 21U) & 0x1FU;
    const auto rt = (instruction >> 16U) & 0x1FU;
    const auto rd = (instruction >> 11U) & 0x1FU;
    const auto shamt = (instruction >> 6U) & 0x1FU;
    const auto funct = instruction & 0x3FU;
    const auto imm_u = static_cast<std::uint16_t>(instruction & 0xFFFFU);
    const auto imm_s = static_cast<std::int16_t>(imm_u);
    const auto signed_imm = static_cast<std::int64_t>(imm_s);
    const auto a = gpr_[rs].lo;
    const auto b = gpr_[rt].lo;
    bool supported = true;
    bool memory_ok = true;
    std::uint32_t memory_address = static_cast<std::uint32_t>(a + static_cast<std::uint64_t>(signed_imm));

    const auto load = [&](std::uint32_t address, std::uint32_t& value) {
        if (!read_word) return false;
        return read_word(address, value);
    };
    const auto store = [&](std::uint32_t address, std::uint32_t value) {
        if (!write_word) return false;
        return write_word(address, value);
    };
    const auto read_byte = [&](std::uint32_t address, std::uint8_t& value) {
        std::uint32_t word = 0;
        if (!load(address & ~std::uint32_t{3}, word)) return false;
        value = static_cast<std::uint8_t>((word >> ((address & 3U) * 8U)) & 0xFFU);
        return true;
    };
    const auto write_byte = [&](std::uint32_t address, std::uint8_t value) {
        const auto aligned = address & ~std::uint32_t{3};
        std::uint32_t word = 0;
        if (!load(aligned, word)) return false;
        const auto shift = (address & 3U) * 8U;
        word = (word & ~(0xFFU << shift)) | (static_cast<std::uint32_t>(value) << shift);
        return store(aligned, word);
    };

    switch (opcode) {
    case 0x00:
        switch (funct) {
        case 0x00: set_gpr(rd, {b << shamt, 0}); break;
        case 0x02: set_gpr(rd, {b >> shamt, 0}); break;
        case 0x03: set_gpr(rd, {static_cast<std::uint64_t>(static_cast<std::int64_t>(b) >> shamt), 0}); break;
        case 0x04: set_gpr(rd, {b << (a & 0x3FU), 0}); break;
        case 0x06: set_gpr(rd, {b >> (a & 0x3FU), 0}); break;
        case 0x07: set_gpr(rd, {static_cast<std::uint64_t>(static_cast<std::int64_t>(b) >> (a & 0x3FU)), 0}); break;
        case 0x08: branch_pending_ = true; branch_target_ = a; break;
        case 0x09: set_gpr(rd, {current_pc + 8U, 0}); branch_pending_ = true; branch_target_ = a; break; // JALR
        case 0x10: set_gpr(rd, {hi_, 0}); break; // MFHI
        case 0x12: set_gpr(rd, {lo_, 0}); break; // MFLO
        case 0x11: hi_ = a; break; // MTHI
        case 0x13: lo_ = a; break; // MTLO
        case 0x18: { // MULT signed 32-bit operands
            const auto product = static_cast<std::int64_t>(static_cast<std::int32_t>(a)) *
                                 static_cast<std::int64_t>(static_cast<std::int32_t>(b));
            const auto bits = static_cast<std::uint64_t>(product);
            lo_ = static_cast<std::uint32_t>(bits);
            hi_ = static_cast<std::uint32_t>(bits >> 32U);
            break;
        }
        case 0x19: { // MULTU unsigned 32-bit operands
            const auto product = static_cast<std::uint64_t>(static_cast<std::uint32_t>(a)) *
                                 static_cast<std::uint64_t>(static_cast<std::uint32_t>(b));
            lo_ = static_cast<std::uint32_t>(product);
            hi_ = static_cast<std::uint32_t>(product >> 32U);
            break;
        }
        case 0x1A: { // DIV signed 32-bit operands
            const auto dividend = static_cast<std::int32_t>(a);
            const auto divisor = static_cast<std::int32_t>(b);
            if (divisor != 0) {
                if (dividend == INT32_MIN && divisor == -1) {
                    lo_ = static_cast<std::uint32_t>(INT32_MIN); hi_ = 0;
                } else {
                    lo_ = static_cast<std::uint32_t>(dividend / divisor);
                    hi_ = static_cast<std::uint32_t>(dividend % divisor);
                }
            }
            break;
        }
        case 0x1B: { // DIVU unsigned 32-bit operands
            const auto dividend = static_cast<std::uint32_t>(a);
            const auto divisor = static_cast<std::uint32_t>(b);
            if (divisor != 0) { lo_ = dividend / divisor; hi_ = dividend % divisor; }
            break;
        }
        case 0x0A: if (b == 0) set_gpr(rd, {a, gpr_[rs].hi}); break; // MOVZ
        case 0x0B: if (b != 0) set_gpr(rd, {a, gpr_[rs].hi}); break; // MOVN
        case 0x0F: break; // SYNC (ordering is provided by the single-threaded interpreter)
        case 0x21: { const auto result = static_cast<std::uint32_t>(a) + static_cast<std::uint32_t>(b); set_gpr(rd, {static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(result))), 0}); break; }
        case 0x2C: set_gpr(rd, {a + b, 0}); break; // DADD (overflow exception not modeled)
        case 0x2D: set_gpr(rd, {a + b, 0}); break;
        case 0x23: { const auto result = static_cast<std::uint32_t>(a) - static_cast<std::uint32_t>(b); set_gpr(rd, {static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(result))), 0}); break; }
        case 0x2E: set_gpr(rd, {a - b, 0}); break; // DSUB (overflow exception not modeled)
        case 0x2F: set_gpr(rd, {a - b, 0}); break;
        case 0x24: set_gpr(rd, {a & b, 0}); break;
        case 0x25: set_gpr(rd, {a | b, 0}); break;
        case 0x26: set_gpr(rd, {a ^ b, 0}); break;
        case 0x27: set_gpr(rd, {~(a | b), 0}); break;
        case 0x2A: set_gpr(rd, {static_cast<std::int64_t>(a) < static_cast<std::int64_t>(b) ? 1ULL : 0ULL, 0}); break;
        case 0x2B: set_gpr(rd, {a < b ? 1ULL : 0ULL, 0}); break;
        case 0x38: set_gpr(rd, {b << shamt, 0}); break;
        case 0x3A: set_gpr(rd, {b >> shamt, 0}); break;
        case 0x3B: set_gpr(rd, {static_cast<std::uint64_t>(static_cast<std::int64_t>(b) >> shamt), 0}); break;
        case 0x3C: set_gpr(rd, {b << (shamt + 32U), 0}); break;
        case 0x3E: set_gpr(rd, {b >> (shamt + 32U), 0}); break;
        case 0x3F: set_gpr(rd, {static_cast<std::uint64_t>(static_cast<std::int64_t>(b) >> (shamt + 32U)), 0}); break;
        default: supported = false; break;
        }
        break;
    case 0x10: { // COP0: limited register transfers; exception/privilege semantics remain unimplemented
        const auto cop_rs = rs;
        if (cop_rs == 0x00U) { // MFC0
            const auto value = cp0_[rd];
            set_gpr(rt, {static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(value))), 0});
        } else if (cop_rs == 0x04U) { // MTC0
            cp0_[rd] = static_cast<std::uint32_t>(gpr_[rt].lo);
        } else {
            supported = false;
        }
        break;
    }
    case 0x01: { // REGIMM: signed branches and conditional link variants
        const auto subop = rt;
        const auto signed_a = static_cast<std::int64_t>(a);
        bool recognized = true;
        bool take = false;
        bool link = false;
        switch (subop) {
        case 0x00: take = signed_a < 0; break; // BLTZ
        case 0x01: take = signed_a >= 0; break; // BGEZ
        case 0x10: take = signed_a < 0; link = true; break; // BLTZAL
        case 0x11: take = signed_a >= 0; link = true; break; // BGEZAL
        default: recognized = false; break;
        }
        supported = recognized;
        if (recognized) {
            if (link && take) set_gpr(31, {current_pc + 8U, 0});
            branch_pending_ = true;
            branch_target_ = take
                ? current_pc + 4U + static_cast<std::uint64_t>(signed_imm * 4)
                : current_pc + 8U;
        }
        break;
    }
    case 0x02:
    case 0x03: {
        if (opcode == 0x03) set_gpr(31, {current_pc + 8U, 0});
        const std::uint64_t region = (current_pc + 4U) & ~std::uint64_t{0x0FFFFFFF};
        branch_pending_ = true;
        branch_target_ = region | (static_cast<std::uint64_t>(instruction & 0x03FFFFFFU) << 2U);
        break;
    }
    case 0x08: // ADDI (overflow traps not modeled)
    case 0x09: { // ADDIU: 32-bit result, sign-extended to 64 bits
        const auto result = static_cast<std::uint32_t>(a) + static_cast<std::uint32_t>(signed_imm);
        set_gpr(rt, {static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(result))), 0});
        break;
    }
    case 0x18: // DADDI (overflow traps not modeled)
    case 0x19: // DADDIU
        set_gpr(rt, {a + static_cast<std::uint64_t>(signed_imm), 0});
        break;
    case 0x1E: {
        memory_ok = (memory_address & 0xFU) == 0U && static_cast<bool>(read_word);
        std::uint32_t words[4]{};
        for (unsigned i = 0; memory_ok && i < 4; ++i) memory_ok = read_word(memory_address + i * 4U, words[i]);
        if (memory_ok && rt != 0) set_gpr(rt, {static_cast<std::uint64_t>(words[0]) | (static_cast<std::uint64_t>(words[1]) << 32U), static_cast<std::uint64_t>(words[2]) | (static_cast<std::uint64_t>(words[3]) << 32U)});
        break;
    }
    case 0x1F: {
        memory_ok = (memory_address & 0xFU) == 0U && static_cast<bool>(write_word);
        const std::uint32_t words[4] = {static_cast<std::uint32_t>(b), static_cast<std::uint32_t>(b >> 32U), static_cast<std::uint32_t>(gpr_[rt].hi), static_cast<std::uint32_t>(gpr_[rt].hi >> 32U)};
        for (unsigned i = 0; memory_ok && i < 4; ++i) memory_ok = write_word(memory_address + i * 4U, words[i]);
        break;
    }
    case 0x0A: set_gpr(rt, {static_cast<std::int64_t>(a) < signed_imm ? 1ULL : 0ULL, 0}); break;
    case 0x0B: set_gpr(rt, {a < static_cast<std::uint64_t>(signed_imm) ? 1ULL : 0ULL, 0}); break;
    case 0x0C: set_gpr(rt, {a & imm_u, 0}); break;
    case 0x0D: set_gpr(rt, {a | imm_u, 0}); break;
    case 0x0E: set_gpr(rt, {a ^ imm_u, 0}); break;
    case 0x0F: { const auto result = static_cast<std::uint32_t>(imm_u) << 16U; set_gpr(rt, {static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(result))), 0}); break; }
    case 0x06:
    case 0x07: {
        const bool take = opcode == 0x06 ? static_cast<std::int64_t>(a) <= 0 : static_cast<std::int64_t>(a) > 0;
        branch_pending_ = true;
        branch_target_ = take ? current_pc + 4U + static_cast<std::uint64_t>(signed_imm * 4) : current_pc + 8U;
        break;
    }
    case 0x04:
    case 0x05: {
        const bool equal = a == b;
        const bool take = opcode == 0x04 ? equal : !equal;
        branch_pending_ = true;
        branch_target_ = take ? current_pc + 4U + static_cast<std::uint64_t>(signed_imm * 4) : current_pc + 8U;
        break;
    }
    case 0x22: { // LWL: merge the addressed word's left part into a register (little-endian)
        std::uint32_t word = 0;
        const auto lane = memory_address & 3U;
        memory_ok = load(memory_address & ~std::uint32_t{3}, word);
        if (memory_ok) {
            const auto shift = (3U - lane) * 8U;
            const auto preserve = 0x00FFFFFFU >> (lane * 8U);
            const auto merged = (static_cast<std::uint32_t>(b) & preserve) | (word << shift);
            set_gpr(rt, {static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(merged))), 0});
        }
        break;
    }
    case 0x26: { // LWR: merge the addressed word's right part into a register (little-endian)
        std::uint32_t word = 0;
        const auto lane = memory_address & 3U;
        memory_ok = load(memory_address & ~std::uint32_t{3}, word);
        if (memory_ok) {
            const auto shift = lane * 8U;
            const auto preserve = lane == 0U ? 0U : (0xFFFFFFFFU << ((4U - lane) * 8U));
            const auto merged = (static_cast<std::uint32_t>(b) & preserve) | (word >> shift);
            set_gpr(rt, {static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(merged))), 0});
        }
        break;
    }
    case 0x21: // LH: load signed halfword
    case 0x25: { // LHU: load unsigned halfword
        std::uint32_t word = 0;
        memory_ok = load(memory_address & ~std::uint32_t{3}, word);
        if (memory_ok) {
            const auto shift = (memory_address & 2U) * 8U;
            const auto half = static_cast<std::uint16_t>((word >> shift) & 0xFFFFU);
            set_gpr(rt, {opcode == 0x21
                ? static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int16_t>(half)))
                : static_cast<std::uint64_t>(half), 0});
        }
        break;
    }
    case 0x20: // LB
    case 0x24: { // LBU
        std::uint8_t value = 0;
        memory_ok = read_byte(memory_address, value);
        if (memory_ok) set_gpr(rt, {opcode == 0x20 ? static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int8_t>(value))) : value, 0});
        break;
    }
    case 0x23:
    case 0x27: { // LW / LWU
        std::uint32_t value = 0;
        memory_ok = (memory_address & 3U) == 0U && load(memory_address, value);
        if (memory_ok) set_gpr(rt, {opcode == 0x23 ? static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(value))) : value, 0});
        break;
    }
    case 0x29: { // SH: store halfword while preserving adjacent bytes
        std::uint32_t word = 0;
        const auto aligned = memory_address & ~std::uint32_t{3};
        memory_ok = (memory_address & 1U) == 0U && load(aligned, word);
        if (memory_ok) {
            const auto shift = (memory_address & 2U) * 8U;
            const auto mask = static_cast<std::uint32_t>(0xFFFFU << shift);
            word = (word & ~mask) | (static_cast<std::uint32_t>(b & 0xFFFFU) << shift);
            memory_ok = store(aligned, word);
        }
        break;
    }
    case 0x28: // SB
        memory_ok = write_byte(memory_address, static_cast<std::uint8_t>(b & 0xFFU));
        break;
    case 0x2B: // SW
        memory_ok = (memory_address & 3U) == 0U && store(memory_address, static_cast<std::uint32_t>(b));
        break;
    case 0x37: { // LD: load a 64-bit doubleword
        std::uint32_t low = 0, high = 0;
        memory_ok = (memory_address & 7U) == 0U
                 && load(memory_address, low) && load(memory_address + 4U, high);
        if (memory_ok) set_gpr(rt, {static_cast<std::uint64_t>(low) | (static_cast<std::uint64_t>(high) << 32U), 0});
        break;
    }
    case 0x3F: { // SD: store a 64-bit doubleword
        memory_ok = (memory_address & 7U) == 0U
                 && store(memory_address, static_cast<std::uint32_t>(b))
                 && store(memory_address + 4U, static_cast<std::uint32_t>(b >> 32U));
        break;
    }
    default: supported = false; break;
    }

    if (applying_previous_branch) pc_ = previous_target;
    else pc_ = current_pc + 4U;

    if (!supported || !memory_ok) {
        if (error) {
            std::ostringstream out;
            if (!supported) {
                out << "EE core foundation: unsupported opcode=0x" << std::hex << opcode
                    << " funct=0x" << funct << " at PC=0x" << current_pc;
            } else {
                out << "EE memory access failed at address=0x" << std::hex << memory_address
                    << " for opcode=0x" << opcode << " at PC=0x" << current_pc;
            }
            *error = out.str();
        }
        return false;
    }
    return true;
}
} // namespace retroshelf::ee
