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

bool EeCpu::step(std::uint32_t instruction, std::string* error) {
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

    switch (opcode) {
    case 0x00: // SPECIAL
        switch (funct) {
        case 0x00: set_gpr(rd, {b << shamt, 0}); break; // SLL
        case 0x02: set_gpr(rd, {b >> shamt, 0}); break; // SRL
        case 0x08: // JR
            branch_pending_ = true;
            branch_target_ = a;
            break;
        case 0x21: // ADDU (prototype 64-bit semantics)
        case 0x2D: set_gpr(rd, {a + b, 0}); break; // DADDU
        case 0x23:
        case 0x2F: set_gpr(rd, {a - b, 0}); break; // SUBU / DSUBU
        case 0x24: set_gpr(rd, {a & b, 0}); break; // AND
        case 0x25: set_gpr(rd, {a | b, 0}); break; // OR
        case 0x26: set_gpr(rd, {a ^ b, 0}); break; // XOR
        case 0x27: set_gpr(rd, {~(a | b), 0}); break; // NOR
        case 0x2A:
            set_gpr(rd, {static_cast<std::int64_t>(a) < static_cast<std::int64_t>(b) ? 1ULL : 0ULL, 0});
            break;
        case 0x2B: set_gpr(rd, {a < b ? 1ULL : 0ULL, 0}); break;
        default: supported = false; break;
        }
        break;
    case 0x02: // J
    case 0x03: { // JAL
        if (opcode == 0x03) set_gpr(31, {current_pc + 8U, 0});
        const std::uint64_t region = (current_pc + 4U) & ~std::uint64_t{0x0FFFFFFF};
        branch_pending_ = true;
        branch_target_ = region | (static_cast<std::uint64_t>(instruction & 0x03FFFFFFU) << 2U);
        break;
    }
    case 0x08: // ADDI (overflow exceptions not modeled)
    case 0x09: // ADDIU
    case 0x19: // DADDIU
        set_gpr(rt, {a + static_cast<std::uint64_t>(signed_imm), 0});
        break;
    case 0x0A: set_gpr(rt, {static_cast<std::int64_t>(a) < signed_imm ? 1ULL : 0ULL, 0}); break; // SLTI
    case 0x0B: set_gpr(rt, {a < static_cast<std::uint64_t>(signed_imm) ? 1ULL : 0ULL, 0}); break; // SLTIU
    case 0x0C: set_gpr(rt, {a & imm_u, 0}); break; // ANDI
    case 0x0D: set_gpr(rt, {a | imm_u, 0}); break; // ORI
    case 0x0E: set_gpr(rt, {a ^ imm_u, 0}); break; // XORI
    case 0x0F: set_gpr(rt, {static_cast<std::uint64_t>(imm_u) << 16U, 0}); break; // LUI
    case 0x06: // BLEZ
    case 0x07: { // BGTZ
        const bool take = opcode == 0x06
            ? static_cast<std::int64_t>(a) <= 0
            : static_cast<std::int64_t>(a) > 0;
        branch_pending_ = true;
        branch_target_ = take
            ? current_pc + 4U + static_cast<std::uint64_t>(signed_imm * 4)
            : current_pc + 8U;
        break;
    }
    case 0x04: // BEQ
    case 0x05: { // BNE
        const bool equal = a == b;
        const bool take = opcode == 0x04 ? equal : !equal;
        branch_pending_ = true;
        branch_target_ = take
            ? current_pc + 4U + static_cast<std::uint64_t>(signed_imm * 4)
            : current_pc + 8U;
        break;
    }
    default:
        supported = false;
        break;
    }

    if (applying_previous_branch) {
        pc_ = previous_target;
    } else {
        pc_ = current_pc + 4U;
    }

    if (!supported) {
        if (error) {
            std::ostringstream out;
            out << "EE core foundation: unsupported opcode=0x" << std::hex
                << opcode << " funct=0x" << funct
                << " at PC=0x" << current_pc;
            *error = out.str();
        }
        return false;
    }
    return true;
}

} // namespace retroshelf::ee
