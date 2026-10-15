#include "retroshelf/cpu.hpp"
#include "retroshelf/ps2_ee_memory_map.hpp"

#include <sstream>
#include <vector>

namespace retroshelf {
Cpu::Cpu() { reset(); }
void Cpu::reset() {
    gpr_.fill(0); memory_.fill(0); active_ram_ = nullptr; active_ram_base_ = 0; active_ee_memory_ = nullptr; pc_ = 0; next_pc_ = 4;
    delay_slot_pending_ = false; delay_branch_pc_ = 0; status_ = 0; cause_ = 0; epc_ = 0;
}
std::uint64_t Cpu::reg(std::size_t index) const { return index < RegisterCount ? gpr_[index] : 0; }
void Cpu::set_reg(std::size_t index, std::uint64_t value) {
    if (index > 0 && index < RegisterCount) gpr_[index] = value;
    gpr_[0] = 0;
}
std::uint64_t Cpu::pc() const { return pc_; }
void Cpu::set_pc(std::uint64_t value) { pc_ = value; next_pc_ = value + 4; delay_slot_pending_ = false; }
std::uint32_t Cpu::cop0_status() const { return status_; }
std::uint32_t Cpu::cop0_cause() const { return cause_; }
std::uint64_t Cpu::cop0_epc() const { return epc_; }
bool Cpu::in_delay_slot() const { return delay_slot_pending_; }
std::uint64_t Cpu::next_pc() const { return next_pc_; }

void Cpu::raise_exception(std::uint32_t code, std::uint64_t fault_pc, bool in_delay_slot) {
    if ((status_ & StatusExl) == 0U) {
        if (in_delay_slot) { epc_ = delay_branch_pc_; cause_ |= CauseBd; }
        else { epc_ = fault_pc; cause_ &= ~CauseBd; }
    }
    cause_ = (cause_ & ~(0x1FU << 2U)) | (code << 2U);
    status_ |= StatusExl;
    pc_ = GeneralExceptionVector;
    next_pc_ = pc_ + 4;
    delay_slot_pending_ = false;
}

bool Cpu::step(std::uint32_t instruction, std::string* error) {
    const auto opcode = (instruction >> 26U) & 0x3FU;
    const auto rs = (instruction >> 21U) & 0x1FU;
    const auto rt = (instruction >> 16U) & 0x1FU;
    const auto rd = (instruction >> 11U) & 0x1FU;
    const auto funct = instruction & 0x3FU;
    const auto imm = static_cast<std::int16_t>(instruction & 0xFFFFU);
    const auto simm = static_cast<std::int64_t>(imm);
    const auto sreg = [this](std::size_t i) { return static_cast<std::int64_t>(reg(i)); };

    const std::uint64_t cur_pc = pc_;
    const bool in_delay = delay_slot_pending_;
    const std::uint64_t branch_base = pc_ + 4; // branch/jump targets are relative to the delay slot

    const bool is_jr = opcode == 0x00 && (funct == 0x08 || funct == 0x09);
    const bool is_branch = (opcode >= 0x01 && opcode <= 0x07) || is_jr;
    if (is_branch && in_delay) {
        if (error) *error = "Branch or jump inside a branch delay slot is not supported";
        return false;
    }

    // Advance: the next instruction comes from next_pc_ (which a branch may redirect).
    pc_ = next_pc_;
    next_pc_ = pc_ + 4;
    delay_slot_pending_ = false;

    const auto unsupported = [&]() {
        if (error) {
            std::ostringstream out;
            out << "Unsupported instruction opcode=0x" << std::hex << opcode << " funct=0x" << funct;
            *error = out.str();
        }
        raise_exception(ExcReservedInstruction, cur_pc, in_delay);
        return false;
    };
    // Schedule a branch: delay slot executes next, then control moves to target (if taken).
    const auto branch = [&](bool taken, std::uint64_t target) {
        delay_slot_pending_ = true;
        delay_branch_pc_ = cur_pc;
        if (taken) next_pc_ = target;
        return true;
    };
    const std::uint64_t rel_target = branch_base + static_cast<std::uint64_t>(simm * 4);

    switch (opcode) {
    case 0x00:
        switch (funct) {
        case 0x21: set_reg(rd, reg(rs) + reg(rt)); return true; // ADDU
        case 0x23: set_reg(rd, reg(rs) - reg(rt)); return true; // SUBU
        case 0x24: set_reg(rd, reg(rs) & reg(rt)); return true; // AND
        case 0x25: set_reg(rd, reg(rs) | reg(rt)); return true; // OR
        case 0x26: set_reg(rd, reg(rs) ^ reg(rt)); return true; // XOR
        case 0x27: set_reg(rd, ~(reg(rs) | reg(rt))); return true; // NOR
        case 0x2A: set_reg(rd, sreg(rs) < sreg(rt) ? 1U : 0U); return true; // SLT
        case 0x2B: set_reg(rd, reg(rs) < reg(rt) ? 1U : 0U); return true; // SLTU
        case 0x00: set_reg(rd, static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(static_cast<std::uint32_t>(reg(rt) << ((instruction >> 6U) & 0x1FU)))))); return true; // SLL/NOP
        case 0x02: set_reg(rd, static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(static_cast<std::uint32_t>(reg(rt)) >> ((instruction >> 6U) & 0x1FU))))); return true; // SRL
        case 0x03: set_reg(rd, static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(reg(rt))) >> ((instruction >> 6U) & 0x1FU))); return true; // SRA
        case 0x04: set_reg(rd, static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(static_cast<std::uint32_t>(reg(rt) << (reg(rs) & 0x1FU)))))); return true; // SLLV
        case 0x06: set_reg(rd, static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(static_cast<std::uint32_t>(reg(rt)) >> (reg(rs) & 0x1FU))))); return true; // SRLV
        case 0x07: set_reg(rd, static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(reg(rt))) >> (reg(rs) & 0x1FU))); return true; // SRAV
        case 0x08: return branch(true, reg(rs)); // JR
        case 0x09: { // JALR
            const auto target = reg(rs); // read before writing rd (rd may equal rs)
            set_reg(rd, cur_pc + 8);
            return branch(true, target);
        }
        default: break;
        }
        break;
    case 0x01: { // REGIMM subset: BLTZ, BGEZ, BLTZAL, BGEZAL
        if (rt != 0x00 && rt != 0x01 && rt != 0x10 && rt != 0x11) return unsupported();
        const bool taken = (rt & 1U) ? sreg(rs) >= 0 : sreg(rs) < 0;
        if (rt >= 0x10) set_reg(31, cur_pc + 8); // link is written even when not taken
        return branch(taken, rel_target);
    }
    case 0x02: case 0x03: { // J / JAL
        if (opcode == 0x03) set_reg(31, cur_pc + 8);
        return branch(true, (branch_base & ~0x0FFFFFFFULL) | (static_cast<std::uint64_t>(instruction & 0x03FFFFFFU) << 2U));
    }
    case 0x04: return branch(reg(rs) == reg(rt), rel_target); // BEQ
    case 0x05: return branch(reg(rs) != reg(rt), rel_target); // BNE
    case 0x06: return branch(sreg(rs) <= 0, rel_target); // BLEZ
    case 0x07: return branch(sreg(rs) > 0, rel_target); // BGTZ
    case 0x09: // ADDIU
        set_reg(rt, reg(rs) + static_cast<std::uint64_t>(simm));
        return true;
    case 0x0A: // SLTI
        set_reg(rt, sreg(rs) < simm ? 1U : 0U);
        return true;
    case 0x0B: // SLTIU (the immediate is sign-extended, then compared unsigned)
        set_reg(rt, reg(rs) < static_cast<std::uint64_t>(simm) ? 1U : 0U);
        return true;
    case 0x0C: // ANDI
        set_reg(rt, reg(rs) & static_cast<std::uint16_t>(instruction & 0xFFFFU));
        return true;
    case 0x0D: // ORI
        set_reg(rt, reg(rs) | static_cast<std::uint16_t>(instruction & 0xFFFFU));
        return true;
    case 0x0E: // XORI
        set_reg(rt, reg(rs) ^ static_cast<std::uint16_t>(instruction & 0xFFFFU));
        return true;
    case 0x0F: // LUI
        set_reg(rt, static_cast<std::uint64_t>(instruction & 0xFFFFU) << 16U);
        return true;
    case 0x20: case 0x21: case 0x23: case 0x24: case 0x25: case 0x27:
    case 0x28: case 0x29: case 0x2B: case 0x37: case 0x3F: { // LB/LH/LW/LBU/LHU/LWU/SB/SH/SW/LD/SD
        const bool store = opcode == 0x28 || opcode == 0x29 || opcode == 0x2B || opcode == 0x3F;
        const std::size_t width = (opcode == 0x20 || opcode == 0x24 || opcode == 0x28) ? 1U :
                                  (opcode == 0x21 || opcode == 0x25 || opcode == 0x29) ? 2U :
                                  (opcode == 0x37 || opcode == 0x3F) ? 8U : 4U;
        const std::uint64_t address = reg(rs) + static_cast<std::uint64_t>(simm);
        std::uint8_t* bytes = nullptr;
        std::size_t offset = 0;
        const auto fault = [&](const char* message) {
            if (error) *error = message;
            raise_exception(store ? ExcAddressErrorStore : ExcAddressErrorLoad, cur_pc, in_delay);
            return false;
        };
        if (active_ee_memory_ != nullptr) {
            std::uint64_t value = 0;
            if (store) {
                if (!active_ee_memory_->write(static_cast<std::uint32_t>(address), width, reg(rt), nullptr))
                    return fault("Unaligned or unmapped EE RAM store");
            } else {
                if (!active_ee_memory_->read(static_cast<std::uint32_t>(address), width, value, nullptr))
                    return fault("Unaligned or unmapped EE RAM load");
                if (opcode == 0x20) set_reg(rt, static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int8_t>(value))));
                else if (opcode == 0x21) set_reg(rt, static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int16_t>(value))));
                else if (opcode == 0x23) set_reg(rt, static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(value))));
                else set_reg(rt, value);
            }
            return true;
        } else if (active_ram_ != nullptr) {
            if (address < active_ram_base_) return fault("Data access is below external RAM");
            const std::uint64_t relative = address - active_ram_base_;
            if ((address & (width - 1U)) != 0U || relative > active_ram_->size() ||
                active_ram_->size() - static_cast<std::size_t>(relative) < width)
                return fault("Unaligned or out-of-range external RAM access");
            offset = static_cast<std::size_t>(relative);
            bytes = active_ram_->data();
        } else {
            if ((address & (width - 1U)) != 0U || address > TestMemorySize - width)
                return fault("Unaligned or out-of-range test RAM access");
            offset = static_cast<std::size_t>(address);
            bytes = memory_.data();
        }

        if (store) {
            const std::uint64_t value = reg(rt);
            for (std::size_t i = 0; i < width; ++i)
                bytes[offset + i] = static_cast<std::uint8_t>((value >> (8U * i)) & 0xFFU);
        } else {
            std::uint64_t value = 0;
            for (std::size_t i = 0; i < width; ++i)
                value |= static_cast<std::uint64_t>(bytes[offset + i]) << (8U * i);
            if (opcode == 0x20) set_reg(rt, static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int8_t>(value))));
            else if (opcode == 0x21) set_reg(rt, static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int16_t>(value))));
            else if (opcode == 0x23) set_reg(rt, static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(value))));
            else set_reg(rt, value); // LBU/LHU/LWU zero-extend; LD preserves all 64 bits
        }
        return true;
    }
    default: break;
    }
    return unsupported();
}

bool Cpu::step_from_ram(std::vector<std::uint8_t>& ram,
                        std::uint32_t ram_base,
                        std::string* error) {
    if ((pc_ & 3U) != 0U) {
        if (error) *error = "Unaligned instruction fetch";
        return false;
    }
    if (pc_ < ram_base) {
        if (error) *error = "Instruction fetch is outside RAM";
        return false;
    }
    const std::uint64_t offset = pc_ - ram_base;
    if (offset > ram.size() ||
        ram.size() - static_cast<std::size_t>(offset) < sizeof(std::uint32_t)) {
        if (error) *error = "Instruction fetch is outside RAM";
        return false;
    }
    const auto index = static_cast<std::size_t>(offset);
    const std::uint32_t instruction =
        static_cast<std::uint32_t>(ram[index]) |
        (static_cast<std::uint32_t>(ram[index + 1]) << 8U) |
        (static_cast<std::uint32_t>(ram[index + 2]) << 16U) |
        (static_cast<std::uint32_t>(ram[index + 3]) << 24U);
    if (error) error->clear();
    active_ram_ = &ram;
    active_ram_base_ = ram_base;
    const bool succeeded = step(instruction, error);
    active_ram_ = nullptr;
    active_ram_base_ = 0;
    return succeeded;
}

bool Cpu::step_from_ee_memory(Ps2EeMemoryMap& memory, std::string* error) {
    if ((pc_ & 3U) != 0U) {
        if (error) *error = "Unaligned instruction fetch";
        return false;
    }
    std::uint64_t fetched = 0;
    if (!memory.read(static_cast<std::uint32_t>(pc_), 4, fetched, error)) {
        if (error && error->empty()) *error = "Instruction fetch is outside mapped EE RAM";
        return false;
    }
    active_ee_memory_ = &memory;
    const bool succeeded = step(static_cast<std::uint32_t>(fetched), error);
    active_ee_memory_ = nullptr;
    return succeeded;
}

} // namespace retroshelf
