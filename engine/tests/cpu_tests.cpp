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

    cpu.set_reg(8, 0xF0F0); cpu.set_reg(9, 0x0FF0);
    constexpr std::uint32_t and_ins = (8U << 21U) | (9U << 16U) | (10U << 11U) | 0x24U;
    constexpr std::uint32_t or_ins = (8U << 21U) | (9U << 16U) | (11U << 11U) | 0x25U;
    constexpr std::uint32_t xor_ins = (8U << 21U) | (9U << 16U) | (12U << 11U) | 0x26U;
    constexpr std::uint32_t nor_ins = (8U << 21U) | (9U << 16U) | (13U << 11U) | 0x27U;
    check(cpu.step(and_ins) && cpu.reg(10) == 0x00F0, "AND computes bitwise intersection");
    check(cpu.step(or_ins) && cpu.reg(11) == 0xFFF0, "OR computes bitwise union");
    check(cpu.step(xor_ins) && cpu.reg(12) == 0xFF00, "XOR computes differing bits");
    check(cpu.step(nor_ins) && cpu.reg(13) == ~std::uint64_t{0xFFF0}, "NOR complements OR");

    cpu.set_reg(8, static_cast<std::uint64_t>(-2LL)); cpu.set_reg(9, 1);
    constexpr std::uint32_t slt = (8U << 21U) | (9U << 16U) | (14U << 11U) | 0x2AU;
    constexpr std::uint32_t sltu = (8U << 21U) | (9U << 16U) | (15U << 11U) | 0x2BU;
    check(cpu.step(slt) && cpu.reg(14) == 1, "SLT compares signed values");
    check(cpu.step(sltu) && cpu.reg(15) == 0, "SLTU compares unsigned values");

    cpu.set_reg(8, 4);
    constexpr std::uint32_t subu = (8U << 21U) | (9U << 16U) | (16U << 11U) | 0x23U;
    check(cpu.step(subu) && cpu.reg(16) == 3, "SUBU computes subtraction");
    constexpr std::uint32_t slti = (0x0AU << 26U) | (8U << 21U) | (17U << 16U) | 5U;
    check(cpu.step(slti) && cpu.reg(17) == 1, "SLTI compares register with signed immediate");
    constexpr std::uint32_t addi_negative = (0x08U << 26U) | (8U << 21U) | (18U << 16U) | 0xFFFFU;
    check(cpu.step(addi_negative) && cpu.reg(18) == 3, "ADDI sign-extends a negative immediate");

    cpu.reset(); cpu.set_reg(11, 16); cpu.set_reg(12, 0x12345678);
    constexpr std::uint32_t sw = (0x2BU << 26U) | (11U << 21U) | (12U << 16U);
    constexpr std::uint32_t lw = (0x23U << 26U) | (11U << 21U) | (13U << 16U);
    check(cpu.step(sw), "SW stores a word in test RAM");
    check(cpu.step(lw), "LW loads a word from test RAM");
    check(cpu.reg(13) == 0x12345678, "LW returns the stored word");
    cpu.set_reg(11, 3); std::string error;
    check(!cpu.step(lw, &error), "misaligned LW is rejected");
    check(!error.empty(), "invalid memory access explains why");
    cpu.set_reg(11, retroshelf::Cpu::TestMemorySize); error.clear();
    check(!cpu.step(sw, &error), "out-of-range SW is rejected");

    cpu.reset(); check(cpu.reg(11) == 0 && cpu.pc() == 0, "reset clears registers and PC");
    error.clear(); check(!cpu.step(0xFFFFFFFFU, &error), "unsupported instruction is rejected");
    check(!error.empty(), "unsupported instruction explains why");
    return failures == 0 ? 0 : 1;
}
