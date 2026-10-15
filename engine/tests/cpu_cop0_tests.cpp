#include "retroshelf/cpu.hpp"
#include "retroshelf/system_bus.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using retroshelf::Cpu;

namespace {
constexpr std::uint32_t mfc0(unsigned rt, unsigned rd) { return (0x10U << 26U) | (rt << 16U) | (rd << 11U); }
constexpr std::uint32_t mtc0(unsigned rt, unsigned rd) { return (0x10U << 26U) | (4U << 21U) | (rt << 16U) | (rd << 11U); }
constexpr std::uint32_t Eret = 0x42000018U;
constexpr std::uint32_t Syscall = 0x0000000CU;
constexpr std::uint32_t Break = 0x0000000DU;
constexpr std::uint32_t Lw = (0x23U << 26U) | (11U << 21U) | (13U << 16U);
constexpr std::uint32_t Sw = (0x2BU << 26U) | (11U << 21U) | (12U << 16U);
}

int main() {
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << "\n"; ++failures; }
        else { std::cout << "PASS: " << label << "\n"; }
    };
    std::vector<std::uint8_t> ram(1024, 0);
    retroshelf::SystemBus bus(ram);
    Cpu cpu(bus);
    std::string error;

    check(cpu.cop0(Cpu::Status) == Cpu::ResetStatus, "reset Status is documented simplification (BEV)");
    check(cpu.cop0(Cpu::Cause) == 0 && cpu.cop0(Cpu::Epc) == 0, "reset Cause/EPC are zero");

    // MTC0/MFC0
    cpu.set_reg(8, 0xFFFFFFFFU);
    check(cpu.step(mtc0(8, Cpu::Status)), "MTC0 Status executes");
    check(cpu.cop0(Cpu::Status) == Cpu::StatusWritableMask, "MTC0 Status masks unmodeled bits");
    cpu.set_reg(8, 0x1234);
    check(cpu.step(mtc0(8, Cpu::Epc)) && cpu.cop0(Cpu::Epc) == 0x1234, "MTC0 EPC writes");
    check(cpu.step(mfc0(9, Cpu::Epc)) && cpu.reg(9) == 0x1234, "MFC0 EPC reads");
    cpu.set_reg(8, 0xFFFFFFFFU);
    check(cpu.step(mtc0(8, Cpu::Cause)) && cpu.cop0(Cpu::Cause) == Cpu::CauseWritableMask,
          "MTC0 Cause only writes software interrupt bits");
    cpu.set_reg(8, 0x80000000U);
    cpu.step(mtc0(8, Cpu::Epc));
    check(cpu.step(mfc0(9, Cpu::Epc)) && cpu.reg(9) == 0xFFFFFFFF80000000ULL, "MFC0 sign-extends 32-bit value");
    cpu.set_reg(8, 5);
    check(cpu.step(mtc0(8, Cpu::BadVAddr)) && cpu.cop0(Cpu::BadVAddr) == 0, "BadVAddr is read-only");

    // Unsupported COP0 register / format -> Reserved Instruction
    cpu.reset();
    error.clear();
    check(!cpu.step(mfc0(9, 3), &error) && !error.empty(), "MFC0 of unmodeled register fails");
    check(cpu.exception_code() == Cpu::ReservedInstruction, "unmodeled COP0 register raises Reserved Instruction");
    cpu.reset();
    check(!cpu.step((0x10U << 26U) | (2U << 21U)) && cpu.exception_code() == Cpu::ReservedInstruction,
          "unsupported COP0 format raises Reserved Instruction");

    // SYSCALL with BEV=1
    cpu.reset();
    cpu.set_pc(0x100);
    check(cpu.step(Syscall), "SYSCALL executes");
    check(cpu.exception_code() == Cpu::Syscall, "SYSCALL sets ExcCode 8");
    check(cpu.cop0(Cpu::Epc) == 0x100, "EPC holds SYSCALL address");
    check((cpu.cop0(Cpu::Status) & Cpu::StatusExl) != 0, "exception sets Status.EXL");
    check(cpu.pc() == Cpu::VectorBev, "BEV=1 uses bootstrap vector");

    // EPC not overwritten while EXL=1
    cpu.set_pc(0x200);
    cpu.step(Break);
    check(cpu.exception_code() == Cpu::Breakpoint && cpu.cop0(Cpu::Epc) == 0x100,
          "BREAK sets ExcCode 9 and EPC is preserved while EXL=1");

    // ERET
    check(cpu.step(Eret) && cpu.pc() == 0x100 && (cpu.cop0(Cpu::Status) & Cpu::StatusExl) == 0,
          "ERET returns to EPC and clears EXL");
    error.clear();
    check(!cpu.step(Eret, &error) && !error.empty() && cpu.pc() == 0x104,
          "ERET with EXL clear is rejected without raising");

    // BEV=0 vector
    cpu.reset();
    cpu.set_reg(8, 0);
    cpu.step(mtc0(8, Cpu::Status));
    cpu.set_pc(0x40);
    cpu.step(Syscall);
    check(cpu.pc() == Cpu::VectorNormal && cpu.cop0(Cpu::Epc) == 0x40, "BEV=0 uses normal vector");

    // Address errors
    cpu.reset();
    cpu.set_reg(11, 3);
    cpu.set_pc(0x80);
    error.clear();
    check(!cpu.step(Lw, &error), "misaligned LW fails");
    check(cpu.exception_code() == Cpu::AddressErrorLoad && cpu.cop0(Cpu::BadVAddr) == 3 &&
              cpu.cop0(Cpu::Epc) == 0x80, "misaligned LW raises AdEL with BadVAddr/EPC");
    cpu.reset();
    cpu.set_reg(11, 6);
    check(!cpu.step(Sw) && cpu.exception_code() == Cpu::AddressErrorStore && cpu.cop0(Cpu::BadVAddr) == 6,
          "misaligned SW raises AdES");
    check(ram[6] == 0 && ram[8] == 0, "faulting SW does not modify RAM");

    // Out-of-range aligned access: error, no exception (bus errors not modeled)
    cpu.reset();
    cpu.set_reg(11, ram.size());
    check(!cpu.step(Lw, &error) && cpu.exception_code() == 0 && (cpu.cop0(Cpu::Status) & Cpu::StatusExl) == 0,
          "out-of-range LW fails without an exception");

    // Unsupported instruction
    cpu.reset();
    cpu.set_pc(0x10);
    error.clear();
    check(!cpu.step(0xFFFFFFFFU, &error) && !error.empty(), "unsupported instruction is rejected");
    check(cpu.exception_code() == Cpu::ReservedInstruction && cpu.cop0(Cpu::Epc) == 0x10,
          "unsupported instruction raises Reserved Instruction");
    return failures == 0 ? 0 : 1;
}
