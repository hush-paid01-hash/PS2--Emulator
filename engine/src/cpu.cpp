#include "retroshelf/cpu.hpp"

#include <sstream>
#include <vector>

namespace retroshelf {
Cpu::Cpu() { reset(); }
void Cpu::reset() {
    gpr_.fill(0); memory_.fill(0); pc_ = 0; next_pc_ = 4;
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
        case 0x25: set_reg(rd, reg(rs) | reg(rt)); return true; // OR
        case 0x00: set_reg(rd, reg(rt) << ((instruction >> 6U) & 0x1FU)); return true; // SLL/NOP
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
    case 0x0D: // ORI
        set_reg(rt, reg(rs) | static_cast<std::uint16_t>(instruction & 0xFFFFU));
        return true;
    case 0x0F: // LUI
        set_reg(rt, static_cast<std::uint64_t>(instruction & 0xFFFFU) << 16U);
        return true;
    case 0x23: case 0x2B: { // LW / SW in small test RAM only
        const std::uint64_t address = reg(rs) + static_cast<std::uint64_t>(simm);
        if ((address & 3U) != 0U || address > TestMemorySize - sizeof(std::uint32_t)) {
            if (error) *error = "Unaligned or out-of-range 32-bit memory access";
            raise_exception(opcode == 0x23 ? ExcAddressErrorLoad : ExcAddressErrorStore, cur_pc, in_delay);
            return false;
        }
        const auto offset = static_cast<std::size_t>(address);
        if (opcode == 0x23) {
            const std::uint32_t value = static_cast<std::uint32_t>(memory_[offset]) |
                (static_cast<std::uint32_t>(memory_[offset + 1]) << 8U) |
                (static_cast<std::uint32_t>(memory_[offset + 2]) << 16U) |
                (static_cast<std::uint32_t>(memory_[offset + 3]) << 24U);
            set_reg(rt, static_cast<std::uint64_t>(static_cast<std::int32_t>(value)));
        } else {
            const auto value = static_cast<std::uint32_t>(reg(rt) & 0xFFFFFFFFU);
            memory_[offset] = static_cast<std::uint8_t>(value & 0xFFU);
            memory_[offset + 1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
            memory_[offset + 2] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
            memory_[offset + 3] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
        }
        return true;
    }
    default: break;
    }
    return unsupported();
}

bool Cpu::step_from_ram(const std::vector<std::uint8_t>& ram,
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
    return step(instruction, error);
}
} // namespace retroshelf
