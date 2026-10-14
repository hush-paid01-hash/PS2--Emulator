#include "retroshelf/cpu.hpp"

#include <sstream>

namespace retroshelf {

Cpu::Cpu() { reset(); }

void Cpu::reset() {
    gpr_.fill(0);
    pc_ = 0;
}

std::uint64_t Cpu::reg(std::size_t index) const {
    return index < RegisterCount ? gpr_[index] : 0;
}

void Cpu::set_reg(std::size_t index, std::uint64_t value) {
    if (index > 0 && index < RegisterCount) gpr_[index] = value;
    gpr_[0] = 0; // MIPS register zero is hard-wired to zero.
}

std::uint64_t Cpu::pc() const { return pc_; }
void Cpu::set_pc(std::uint64_t value) { pc_ = value; }

bool Cpu::step(std::uint32_t instruction, std::string* error) {
    const auto opcode = (instruction >> 26U) & 0x3FU;
    const auto rs = (instruction >> 21U) & 0x1FU;
    const auto rt = (instruction >> 16U) & 0x1FU;
    const auto rd = (instruction >> 11U) & 0x1FU;
    const auto funct = instruction & 0x3FU;
    const auto imm = static_cast<std::int16_t>(instruction & 0xFFFFU);

    // Advance PC for this minimal instruction-only test harness.
    pc_ += 4;

    switch (opcode) {
    case 0x00: // SPECIAL / register-register operations
        switch (funct) {
        case 0x21: // ADDU
            set_reg(rd, reg(rs) + reg(rt));
            return true;
        case 0x25: // OR
            set_reg(rd, reg(rs) | reg(rt));
            return true;
        case 0x00: // SLL (also NOP when all fields are zero)
            set_reg(rd, reg(rt) << ((instruction >> 6U) & 0x1FU));
            return true;
        default:
            break;
        }
        break;
    case 0x09: // ADDIU, sign-extended immediate
        set_reg(rt, reg(rs) + static_cast<std::uint64_t>(static_cast<std::int64_t>(imm)));
        return true;
    case 0x0D: // ORI, zero-extended immediate
        set_reg(rt, reg(rs) | static_cast<std::uint16_t>(instruction & 0xFFFFU));
        return true;
    case 0x0F: // LUI
        set_reg(rt, static_cast<std::uint64_t>(instruction & 0xFFFFU) << 16U);
        return true;
    default:
        break;
    }

    if (error) {
        std::ostringstream out;
        out << "Unsupported instruction opcode=0x" << std::hex << opcode
            << " funct=0x" << funct;
        *error = out.str();
    }
    return false;
}

} // namespace retroshelf
