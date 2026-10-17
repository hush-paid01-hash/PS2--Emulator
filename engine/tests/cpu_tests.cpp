#include "retroshelf/cpu.hpp"
#include <cstdint>
#include <iostream>
#include <string>

int main() {
    retroshelf::Cpu cpu;
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << "\n"; ++failures; }
        else { std::cout << "PASS: " << label << "\n"; }
    };

    constexpr std::uint32_t addiu7 = (0x09U << 26U) | (8U << 16U) | 7U;
    constexpr std::uint32_t addiu5 = (0x09U << 26U) | (9U << 16U) | 5U;
    constexpr std::uint32_t addu = (8U << 21U) | (9U << 16U) | (10U << 11U) | 0x21U;
    check(cpu.step(addiu7), "ADDIU executes");
    check(cpu.reg(8) == 7, "ADDIU writes expected value");
    check(cpu.reg(0) == 0, "register zero stays zero");
    check(cpu.step(addiu5), "second ADDIU executes");
    check(cpu.step(addu), "ADDU executes");
    check(cpu.reg(10) == 12, "ADDU computes 7 + 5");
    check(cpu.pc() == 12, "PC advances four bytes per instruction");

    cpu.reset();
    check(cpu.reg(8) == 0 && cpu.pc() == 0, "reset clears CPU state");

    cpu.set_reg(11, 16);
    cpu.set_reg(12, 0x12345678);
    constexpr std::uint32_t sw = (0x2BU << 26U) | (11U << 21U) | (12U << 16U);
    constexpr std::uint32_t lw = (0x23U << 26U) | (11U << 21U) | (13U << 16U);
    check(cpu.step(sw), "SW stores a word in test RAM");
    check(cpu.step(lw), "LW loads a word from test RAM");
    check(cpu.reg(13) == 0x12345678, "LW returns the stored word");

    cpu.set_reg(11, 3);
    std::string error;
    check(!cpu.step(lw, &error), "misaligned LW is rejected");
    check(!error.empty(), "invalid memory access explains why");

    cpu.set_reg(11, retroshelf::Cpu::TestMemorySize);
    error.clear();
    check(!cpu.step(sw, &error), "out-of-range SW is rejected");

    cpu.reset();
    check(cpu.reg(11) == 0 && cpu.pc() == 0, "reset clears registers and PC");
    error.clear();
    check(!cpu.step(0xFFFFFFFFU, &error), "unsupported instruction is rejected");
    check(!error.empty(), "unsupported instruction explains why");

    using retroshelf::Cpu;
    const auto itype = [](std::uint32_t op, std::uint32_t rs, std::uint32_t rt, std::int16_t imm) {
        return (op << 26U) | (rs << 21U) | (rt << 16U) | static_cast<std::uint16_t>(imm);
    };
    const std::uint32_t nop = 0;
    const std::uint32_t set1 = itype(0x09, 0, 20, 1); // ADDIU r20,r0,1 (delay-slot marker)
    const std::uint32_t bad_lw = itype(0x23, 0, 21, 3); // misaligned LW -> AdEL

    const auto rtype = [](std::uint32_t rs, std::uint32_t rt, std::uint32_t rd, std::uint32_t funct) {
        return (rs << 21U) | (rt << 16U) | (rd << 11U) | funct;
    };

    // Immediate logical operations zero-extend their 16-bit masks.
    cpu.reset(); cpu.set_reg(1, 0x12345678);
    check(cpu.step(itype(0x0C, 1, 2, 0x00FF)) && cpu.reg(2) == 0x78,
          "ANDI zero-extends mask and clears other bits");
    check(cpu.step(itype(0x0E, 1, 3, 0x00FF)) && cpu.reg(3) == 0x12345687,
          "XORI zero-extends mask and toggles bits");

    // Signed and unsigned comparisons, including a negative sign-extended immediate.
    cpu.reset(); cpu.set_reg(1, static_cast<std::uint64_t>(-1));
    check(cpu.step(itype(0x0A, 1, 2, 1)) && cpu.reg(2) == 1,
          "SLTI compares signed register values");
    check(cpu.step(itype(0x0A, 1, 3, -2)) && cpu.reg(3) == 0,
          "SLTI handles negative immediates");
    cpu.reset(); cpu.set_reg(1, 1);
    check(cpu.step(itype(0x0B, 1, 2, -1)) && cpu.reg(2) == 1,
          "SLTIU sign-extends immediate before unsigned comparison");
    cpu.set_reg(1, static_cast<std::uint64_t>(-1));
    check(cpu.step(itype(0x0B, 1, 3, -1)) && cpu.reg(3) == 0,
          "SLTIU handles unsigned equality at all-ones");

    // Register-register logical, subtract, and compare instructions.
    cpu.reset(); cpu.set_reg(1, 0xF0F0); cpu.set_reg(2, 0x0FF0);
    check(cpu.step(rtype(1, 2, 3, 0x24)) && cpu.reg(3) == 0x00F0, "AND computes bitwise intersection");
    check(cpu.step(rtype(1, 2, 4, 0x26)) && cpu.reg(4) == 0xFF00, "XOR computes bitwise difference");
    check(cpu.step(rtype(1, 2, 5, 0x27)) && cpu.reg(5) == ~std::uint64_t(0xFFF0), "NOR complements bitwise OR");
    check(cpu.step(rtype(1, 2, 6, 0x23)) && cpu.reg(6) == 0xE100, "SUBU subtracts without signed overflow traps");
    check(cpu.step(rtype(1, 2, 7, 0x2A)) && cpu.reg(7) == 0, "SLT uses signed comparison");
    cpu.set_reg(1, static_cast<std::uint64_t>(-1)); cpu.set_reg(2, 1);
    check(cpu.step(rtype(1, 2, 8, 0x2A)) && cpu.reg(8) == 1, "SLT treats high-bit values as negative");
    check(cpu.step(rtype(1, 2, 9, 0x2B)) && cpu.reg(9) == 0, "SLTU compares register values unsigned");

    // BEQ taken: delay slot runs, then target.
    cpu.reset(); cpu.set_pc(0x100); cpu.set_reg(1, 5); cpu.set_reg(2, 5);
    check(cpu.step(itype(0x04, 1, 2, 3)), "BEQ taken executes");
    check(cpu.in_delay_slot() && cpu.pc() == 0x104, "BEQ leaves PC at delay slot");
    check(cpu.step(set1) && cpu.reg(20) == 1, "delay slot instruction executes");
    check(cpu.pc() == 0x104 + 12 && !cpu.in_delay_slot(), "BEQ taken goes to PC+4+offset*4");

    // BEQ not taken: delay slot still runs, falls through.
    cpu.reset(); cpu.set_pc(0x100); cpu.set_reg(1, 5); cpu.set_reg(2, 6);
    cpu.step(itype(0x04, 1, 2, 3)); cpu.step(set1);
    check(cpu.reg(20) == 1 && cpu.pc() == 0x108, "BEQ not taken falls through after delay slot");

    // BNE taken backward.
    cpu.reset(); cpu.set_pc(0x100); cpu.set_reg(1, 1);
    cpu.step(itype(0x05, 1, 0, -2)); cpu.step(nop);
    check(cpu.pc() == 0x104 - 8, "BNE taken with negative offset");

    // BNE untaken.
    cpu.reset(); cpu.set_pc(0x100);
    cpu.step(itype(0x05, 0, 0, 5)); cpu.step(nop);
    check(cpu.pc() == 0x108, "BNE not taken falls through");

    // BLEZ / BGTZ / BLTZ / BGEZ
    cpu.reset(); cpu.set_pc(0x100); cpu.set_reg(1, 0);
    cpu.step(itype(0x06, 1, 0, 4)); cpu.step(nop);
    check(cpu.pc() == 0x104 + 16, "BLEZ taken on zero");
    cpu.reset(); cpu.set_pc(0x100); cpu.set_reg(1, 0);
    cpu.step(itype(0x07, 1, 0, 4)); cpu.step(nop);
    check(cpu.pc() == 0x108, "BGTZ not taken on zero");
    cpu.reset(); cpu.set_pc(0x100); cpu.set_reg(1, static_cast<std::uint64_t>(-1));
    cpu.step(itype(0x01, 1, 0, 4)); cpu.step(nop);
    check(cpu.pc() == 0x104 + 16, "BLTZ taken on negative");
    cpu.reset(); cpu.set_pc(0x100); cpu.set_reg(1, static_cast<std::uint64_t>(-1));
    cpu.step(itype(0x01, 1, 1, 4)); cpu.step(nop);
    check(cpu.pc() == 0x108, "BGEZ not taken on negative");

    // BGEZAL links even when untaken; BLTZAL taken links to PC+8.
    cpu.reset(); cpu.set_pc(0x100); cpu.set_reg(1, static_cast<std::uint64_t>(-1));
    cpu.step(itype(0x01, 1, 0x11, 4)); cpu.step(nop);
    check(cpu.reg(31) == 0x108 && cpu.pc() == 0x108, "BGEZAL untaken still writes link");
    cpu.reset(); cpu.set_pc(0x100); cpu.set_reg(1, static_cast<std::uint64_t>(-1));
    cpu.step(itype(0x01, 1, 0x10, 4)); cpu.step(nop);
    check(cpu.reg(31) == 0x108 && cpu.pc() == 0x104 + 16, "BLTZAL taken links and branches");

    // J / JAL
    cpu.reset(); cpu.set_pc(0x100);
    cpu.step((0x02U << 26U) | (0x40U)); cpu.step(set1);
    check(cpu.reg(20) == 1 && cpu.pc() == 0x100, "J jumps to 26-bit target << 2 after delay slot");
    cpu.reset(); cpu.set_pc(0x200);
    cpu.step((0x03U << 26U) | 0x10U); cpu.step(nop);
    check(cpu.reg(31) == 0x208 && cpu.pc() == 0x40, "JAL links PC+8 and jumps");

    // JR / JALR
    cpu.reset(); cpu.set_pc(0x100); cpu.set_reg(5, 0x300);
    cpu.step((5U << 21U) | 0x08U); cpu.step(set1);
    check(cpu.reg(20) == 1 && cpu.pc() == 0x300, "JR jumps to register after delay slot");
    cpu.reset(); cpu.set_pc(0x100); cpu.set_reg(5, 0x300);
    cpu.step((5U << 21U) | (6U << 11U) | 0x09U); cpu.step(nop);
    check(cpu.reg(6) == 0x108 && cpu.pc() == 0x300, "JALR links PC+8 and jumps");
    cpu.reset(); cpu.set_pc(0x100); cpu.set_reg(5, 0x300);
    cpu.step((5U << 21U) | (5U << 11U) | 0x09U); cpu.step(nop);
    check(cpu.pc() == 0x300 && cpu.reg(5) == 0x108, "JALR with rd==rs uses old target");

    // Branch inside a delay slot is rejected, not silently mis-executed.
    cpu.reset(); cpu.set_pc(0x100);
    cpu.step(itype(0x04, 0, 0, 3));
    error.clear();
    check(!cpu.step(itype(0x04, 0, 0, 3), &error) && !error.empty(), "branch in delay slot is rejected");

    // Exceptions: normal instruction sets BD=0, EPC=instruction.
    cpu.reset(); cpu.set_pc(0x100);
    check(!cpu.step(bad_lw, &error), "faulting LW reports failure");
    check(cpu.cop0_epc() == 0x100 && (cpu.cop0_cause() & Cpu::CauseBd) == 0, "non-delay-slot fault: EPC=PC, BD=0");
    check(((cpu.cop0_cause() >> 2U) & 0x1FU) == Cpu::ExcAddressErrorLoad, "AdEL exception code");
    check(cpu.pc() == Cpu::GeneralExceptionVector && (cpu.cop0_status() & Cpu::StatusExl) != 0, "exception vectors and sets EXL");

    // Fault in a taken-branch delay slot: EPC = branch address, BD=1, branch cancelled.
    cpu.reset(); cpu.set_pc(0x100); cpu.set_reg(1, 5); cpu.set_reg(2, 5);
    cpu.step(itype(0x04, 1, 2, 3));
    check(!cpu.step(bad_lw, &error), "fault in delay slot reports failure");
    check(cpu.cop0_epc() == 0x100 && (cpu.cop0_cause() & Cpu::CauseBd) != 0, "delay-slot fault: EPC=branch, BD=1");
    check(cpu.pc() == Cpu::GeneralExceptionVector && !cpu.in_delay_slot(), "delay-slot fault discards pending branch");

    // Fault in an untaken-branch delay slot, and in a JR delay slot (store, AdES).
    cpu.reset(); cpu.set_pc(0x200); cpu.set_reg(1, 1);
    cpu.step(itype(0x04, 1, 0, 3));
    cpu.step(0x0000003DU, &error);
    check(cpu.cop0_epc() == 0x200 && (cpu.cop0_cause() & Cpu::CauseBd) != 0 &&
          ((cpu.cop0_cause() >> 2U) & 0x1FU) == Cpu::ExcReservedInstruction, "RI in untaken-branch delay slot sets BD");
    cpu.reset(); cpu.set_pc(0x300); cpu.set_reg(5, 0x400); cpu.set_reg(6, 3);
    cpu.step((5U << 21U) | 0x08U);
    cpu.step(itype(0x2B, 6, 0, 0));
    check(cpu.cop0_epc() == 0x300 && (cpu.cop0_cause() & Cpu::CauseBd) != 0 &&
          ((cpu.cop0_cause() >> 2U) & 0x1FU) == Cpu::ExcAddressErrorStore, "AdES in JR delay slot sets BD, EPC=JR");

    // Delay-slot fault followed by an ordinary instruction: handler instruction runs normally.
    cpu.reset(); cpu.set_pc(0x100); cpu.step(bad_lw);
    check(cpu.step(set1) && cpu.reg(20) == 1 && cpu.pc() == Cpu::GeneralExceptionVector + 4, "execution continues at exception vector");

    // Fixed and variable 32-bit shift operations.
    const auto shift = [](std::uint32_t rs, std::uint32_t rt, std::uint32_t rd,
                          std::uint32_t shamt, std::uint32_t funct) {
        return (rs << 21U) | (rt << 16U) | (rd << 11U) | (shamt << 6U) | funct;
    };
    cpu.reset(); cpu.set_reg(2, 0x00000001U);
    check(cpu.step(shift(0, 2, 3, 4, 0x00)) && cpu.reg(3) == 0x10,
          "SLL shifts left by encoded amount");
    cpu.set_reg(2, 0x80000000U);
    check(cpu.step(shift(0, 2, 4, 1, 0x02)) && cpu.reg(4) == 0x40000000ULL,
          "SRL shifts logically and sign-extends 32-bit result");
    check(cpu.step(shift(0, 2, 5, 1, 0x03)) && cpu.reg(5) == 0xFFFFFFFFC0000000ULL,
          "SRA preserves sign while shifting right");
    cpu.set_reg(1, 3); cpu.set_reg(2, 0x10);
    check(cpu.step(shift(1, 2, 6, 0, 0x04)) && cpu.reg(6) == 0x80,
          "SLLV uses low five bits of rs as shift amount");
    cpu.set_reg(2, 0x80000000U);
    check(cpu.step(shift(1, 2, 7, 0, 0x06)) && cpu.reg(7) == 0x10000000ULL,
          "SRLV shifts right by variable amount");
    check(cpu.step(shift(1, 2, 8, 0, 0x07)) && cpu.reg(8) == 0xFFFFFFFFF0000000ULL,
          "SRAV shifts right arithmetically by variable amount");
    cpu.set_reg(1, 0x20); cpu.set_reg(2, 0x10);
    check(cpu.step(shift(1, 2, 9, 0, 0x04)) && cpu.reg(9) == 0x10,
          "SLLV masks shift amount to five bits");


    // Byte and halfword accesses: sign extension for LB/LH, zero extension for LBU/LHU.
    cpu.reset(); cpu.set_reg(11, 32); cpu.set_reg(12, 0x80);
    check(cpu.step(itype(0x28, 11, 12, 0)), "SB stores a byte");
    check(cpu.step(itype(0x20, 11, 13, 0)) &&
          cpu.reg(13) == 0xFFFFFFFFFFFFFF80ULL, "LB sign-extends negative byte");
    check(cpu.step(itype(0x24, 11, 14, 0)) && cpu.reg(14) == 0x80,
          "LBU zero-extends byte");
    cpu.set_reg(12, 0x8001);
    check(cpu.step(itype(0x29, 11, 12, 2)), "SH stores a halfword");
    check(cpu.step(itype(0x21, 11, 15, 2)) &&
          cpu.reg(15) == 0xFFFFFFFFFFFF8001ULL, "LH sign-extends negative halfword");
    check(cpu.step(itype(0x25, 11, 16, 2)) && cpu.reg(16) == 0x8001,
          "LHU zero-extends halfword");
    cpu.set_reg(11, 33); error.clear();
    check(!cpu.step(itype(0x21, 11, 17, 0), &error) && !error.empty(),
          "misaligned LH faults");
    cpu.reset(); cpu.set_reg(11, 0); cpu.set_reg(12, 0x1234);
    check(cpu.step(itype(0x29, 11, 12, 0)), "SH can store to aligned test RAM");
    check(cpu.step(itype(0x25, 11, 13, 0)) && cpu.reg(13) == 0x1234,
          "LHU reads halfword stored by SH");

    // 32-bit unsigned word and full 64-bit doubleword memory operations.
    cpu.reset(); cpu.set_reg(11, 64); cpu.set_reg(12, 0xFFFFFFFF80000001ULL);
    check(cpu.step(itype(0x2B, 11, 12, 0)), "SW stores low 32 bits of a 64-bit register");
    check(cpu.step(itype(0x27, 11, 13, 0)) && cpu.reg(13) == 0x80000001ULL,
          "LWU zero-extends a word");
    check(cpu.step(itype(0x23, 11, 14, 0)) && cpu.reg(14) == 0xFFFFFFFF80000001ULL,
          "LW sign-extends the same word");

    cpu.reset(); cpu.set_reg(11, 72); cpu.set_reg(12, 0xFEDCBA9876543210ULL);
    check(cpu.step(itype(0x3F, 11, 12, 0)), "SD stores a full 64-bit doubleword");
    check(cpu.step(itype(0x37, 11, 13, 0)) && cpu.reg(13) == 0xFEDCBA9876543210ULL,
          "LD restores all 64 bits");
    cpu.set_reg(11, 76); error.clear();
    check(!cpu.step(itype(0x37, 11, 14, 0), &error) && !error.empty(),
          "misaligned LD faults");
    cpu.reset(); cpu.set_reg(11, Cpu::TestMemorySize - 4); cpu.set_reg(12, 0x1122334455667788ULL);
    error.clear();
    check(!cpu.step(itype(0x3F, 11, 12, 0), &error) && !error.empty(),
          "out-of-range SD faults without writing past test RAM");

    // EE 64-bit immediate arithmetic: sign extension and overflow behavior.
    cpu.reset(); cpu.set_reg(1, 0x100000000ULL);
    check(cpu.step(itype(0x19, 1, 2, -1)) && cpu.reg(2) == 0xFFFFFFFFULL,
          "DADDIU adds a sign-extended immediate in 64 bits");
    cpu.reset(); cpu.set_reg(1, 0x7FFFFFFFFFFFFFF0ULL);
    check(cpu.step(itype(0x18, 1, 2, 15)) && cpu.reg(2) == 0x7FFFFFFFFFFFFFFFULL,
          "DADDI handles signed 64-bit addition without overflow");
    cpu.reset(); cpu.set_reg(1, 0x7FFFFFFFFFFFFFFFULL); cpu.set_reg(2, 123);
    error.clear();
    check(!cpu.step(itype(0x18, 1, 2, 1), &error) && !error.empty(),
          "DADDI traps on signed 64-bit positive overflow");
    check(cpu.reg(2) == 123, "DADDI overflow leaves destination register unchanged");
    check(((cpu.cop0_cause() >> 2U) & 0x1FU) == Cpu::ExcOverflow,
          "DADDI overflow records the overflow exception code");
    cpu.reset(); cpu.set_reg(1, 0xFFFFFFFFFFFFFFFFULL);
    check(cpu.step(itype(0x19, 1, 2, 1)) && cpu.reg(2) == 0,
          "DADDIU wraps around without an overflow exception");

    return failures == 0 ? 0 : 1;
}
