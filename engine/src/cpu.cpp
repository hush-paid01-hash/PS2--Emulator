#include "retroshelf/cpu.hpp"

#include <sstream>
#include <vector>

namespace retroshelf {
Cpu::Cpu(SystemBus& bus) : bus_(bus) { reset(); }
void Cpu::reset() { gpr_.fill(0); pc_ = 0; status_ = ResetStatus; cause_ = 0; epc_ = 0; bad_vaddr_ = 0; }
std::uint64_t Cpu::reg(std::size_t index) const { return index < RegisterCount ? gpr_[index] : 0; }
void Cpu::set_reg(std::size_t index, std::uint64_t value) {
    if (index > 0 && index < RegisterCount) gpr_[index] = value;
    gpr_[0] = 0;
}
std::uint64_t Cpu::pc() const { return pc_; }
void Cpu::set_pc(std::uint64_t value) { pc_ = value; }

bool Cpu::cop0_supported(std::size_t i) const { return i == BadVAddr || i == Status || i == Cause || i == Epc; }
std::uint32_t Cpu::cop0(std::size_t i) const {
    switch (i) {
    case BadVAddr: return bad_vaddr_;
    case Status: return status_;
    case Cause: return cause_;
    case Epc: return epc_;
    default: return 0;
    }
}
std::uint32_t Cpu::exception_code() const { return (cause_ >> 2U) & 0x1FU; }

void Cpu::raise(ExceptionCode code, std::uint64_t faulting_pc, std::uint32_t bad_vaddr,
                bool set_bad_vaddr) {
    if ((status_ & StatusExl) == 0U) epc_ = static_cast<std::uint32_t>(faulting_pc); // EPC kept while EXL=1
    status_ |= StatusExl;
    cause_ = (cause_ & ~(0x1FU << 2U)) | (static_cast<std::uint32_t>(code) << 2U);
    if (set_bad_vaddr) bad_vaddr_ = bad_vaddr;
    pc_ = (status_ & StatusBev) != 0U ? VectorBev : VectorNormal;
}

bool Cpu::step(std::uint32_t instruction, std::string* error) {
    const auto opcode = (instruction >> 26U) & 0x3FU;
    const auto rs = (instruction >> 21U) & 0x1FU;
    const auto rt = (instruction >> 16U) & 0x1FU;
    const auto rd = (instruction >> 11U) & 0x1FU;
    const auto funct = instruction & 0x3FU;
    const auto imm = static_cast<std::int16_t>(instruction & 0xFFFFU);
    const std::uint64_t start_pc = pc_;
    pc_ += 4; // Sequential test harness; branch delay slots are not implemented.

    switch (opcode) {
    case 0x00:
        switch (funct) {
        case 0x0C: raise(Syscall, start_pc); return true; // SYSCALL
        case 0x0D: raise(Breakpoint, start_pc); return true; // BREAK
        case 0x21: set_reg(rd, reg(rs) + reg(rt)); return true; // ADDU
        case 0x25: set_reg(rd, reg(rs) | reg(rt)); return true; // OR
        case 0x00: set_reg(rd, reg(rt) << ((instruction >> 6U) & 0x1FU)); return true; // SLL/NOP
        default: break;
        }
        break;
    case 0x10: { // COP0
        const auto format = rs;
        if ((format == 0x00U || format == 0x04U) && cop0_supported(rd)) {
            if (format == 0x00U) { // MFC0 (sign-extended 32-bit)
                set_reg(rt, static_cast<std::uint64_t>(static_cast<std::int32_t>(cop0(rd))));
            } else { // MTC0
                const auto value = static_cast<std::uint32_t>(reg(rt) & 0xFFFFFFFFU);
                if (rd == Status) status_ = value & StatusWritableMask;
                else if (rd == Cause) cause_ = (cause_ & ~CauseWritableMask) | (value & CauseWritableMask);
                else if (rd == Epc) epc_ = value;
                // BadVAddr is read-only; writes are ignored.
            }
            return true;
        }
        if (instruction == 0x42000018U) // ERET
        {
            if ((status_ & StatusExl) == 0U) { // ERET outside an exception: not defined here
                if (error) *error = "ERET with Status.EXL clear is not supported";
                return false;
            }
            pc_ = static_cast<std::uint64_t>(static_cast<std::int64_t>(static_cast<std::int32_t>(epc_)));
            status_ &= ~StatusExl;
            return true;
        }
        raise(ReservedInstruction, start_pc);
        if (error) *error = "Unsupported COP0 instruction (Reserved Instruction exception raised)";
        return false;
    }
    case 0x09: // ADDIU
        set_reg(rt, reg(rs) + static_cast<std::uint64_t>(static_cast<std::int64_t>(imm)));
        return true;
    case 0x0D: // ORI
        set_reg(rt, reg(rs) | static_cast<std::uint16_t>(instruction & 0xFFFFU));
        return true;
    case 0x0F: // LUI
        set_reg(rt, static_cast<std::uint64_t>(instruction & 0xFFFFU) << 16U);
        return true;
    case 0x23: case 0x2B: { // LW / SW through the shared SystemBus (EE RAM offsets only)
        const std::uint64_t address = reg(rs) + static_cast<std::uint64_t>(static_cast<std::int64_t>(imm));
        if ((address & 3U) != 0U || address > 0xFFFFFFFFULL) {
            raise(opcode == 0x23 ? AddressErrorLoad : AddressErrorStore, start_pc,
                  static_cast<std::uint32_t>(address & 0xFFFFFFFFU), true);
            if (error) *error = "Unaligned or out-of-range 32-bit memory access (Address Error raised)";
            return false;
        }
        const auto bus_address = static_cast<std::uint32_t>(address);
        std::string bus_error;
        if (opcode == 0x23) {
            std::vector<std::uint8_t> bytes;
            if (!bus_.read(bus_address, sizeof(std::uint32_t), bytes, &bus_error)) {
                if (error) *error = "LW failed: " + bus_error;
                return false;
            }
            const std::uint32_t value = static_cast<std::uint32_t>(bytes[0]) |
                (static_cast<std::uint32_t>(bytes[1]) << 8U) |
                (static_cast<std::uint32_t>(bytes[2]) << 16U) |
                (static_cast<std::uint32_t>(bytes[3]) << 24U);
            set_reg(rt, static_cast<std::uint64_t>(static_cast<std::int32_t>(value)));
        } else {
            const auto value = static_cast<std::uint32_t>(reg(rt) & 0xFFFFFFFFU);
            const std::vector<std::uint8_t> bytes{
                static_cast<std::uint8_t>(value & 0xFFU),
                static_cast<std::uint8_t>((value >> 8U) & 0xFFU),
                static_cast<std::uint8_t>((value >> 16U) & 0xFFU),
                static_cast<std::uint8_t>((value >> 24U) & 0xFFU)};
            if (!bus_.write(bus_address, bytes, &bus_error)) {
                if (error) *error = "SW failed: " + bus_error;
                return false;
            }
        }
        return true;
    }
    default: break;
    }
    raise(ReservedInstruction, start_pc);
    if (error) {
        std::ostringstream out;
        out << "Unsupported instruction opcode=0x" << std::hex << opcode << " funct=0x" << funct;
        *error = out.str();
    }
    return false;
}
} // namespace retroshelf
