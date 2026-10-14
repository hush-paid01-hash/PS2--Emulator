#include "retroshelf/cpu.hpp"

#include <sstream>

namespace retroshelf {
Cpu::Cpu() { reset(); }
void Cpu::reset() { gpr_.fill(0); memory_.fill(0); pc_ = 0; }
std::uint64_t Cpu::reg(std::size_t index) const { return index < RegisterCount ? gpr_[index] : 0; }
void Cpu::set_reg(std::size_t index, std::uint64_t value) {
    if (index > 0 && index < RegisterCount) gpr_[index] = value;
    gpr_[0] = 0;
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
    pc_ += 4; // Sequential test harness; branch delay slots are not implemented.

    switch (opcode) {
    case 0x00:
        switch (funct) {
        case 0x21: set_reg(rd, reg(rs) + reg(rt)); return true; // ADDU
        case 0x24: set_reg(rd, reg(rs) & reg(rt)); return true; // AND
        case 0x25: set_reg(rd, reg(rs) | reg(rt)); return true; // OR
        case 0x26: set_reg(rd, reg(rs) ^ reg(rt)); return true; // XOR
        case 0x27: set_reg(rd, ~(reg(rs) | reg(rt))); return true; // NOR
        case 0x2A:
            set_reg(rd, static_cast<std::int64_t>(reg(rs)) < static_cast<std::int64_t>(reg(rt)) ? 1U : 0U);
            return true; // SLT
        case 0x2B: set_reg(rd, reg(rs) < reg(rt) ? 1U : 0U); return true; // SLTU
        case 0x00: set_reg(rd, reg(rt) << ((instruction >> 6U) & 0x1FU)); return true; // SLL/NOP
        default: break;
        }
        break;
    case 0x09: // ADDIU
        set_reg(rt, reg(rs) + static_cast<std::uint64_t>(static_cast<std::int64_t>(imm)));
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
    case 0x23: case 0x2B: { // LW / SW in small test RAM only
        const std::uint64_t address = reg(rs) + static_cast<std::uint64_t>(static_cast<std::int64_t>(imm));
        if ((address & 3U) != 0U || address > TestMemorySize - sizeof(std::uint32_t)) {
            if (error) *error = "Unaligned or out-of-range 32-bit memory access";
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
    if (error) {
        std::ostringstream out;
        out << "Unsupported instruction opcode=0x" << std::hex << opcode << " funct=0x" << funct;
        *error = out.str();
    }
    return false;
}
} // namespace retroshelf
