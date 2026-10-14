#include "retroshelf/ee_cpu.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>

int main() {
    retroshelf::ee::EeCpu cpu;
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << "\n"; ++failures; }
        else { std::cout << "PASS: " << label << "\n"; }
    };
    std::array<std::uint32_t, 16> ram{};
    const auto read_word = [&ram](std::uint32_t address, std::uint32_t& value) {
        if ((address & 3U) || address >= ram.size() * 4U) return false;
        value = ram[address / 4U]; return true;
    };
    const auto write_word = [&ram](std::uint32_t address, std::uint32_t value) {
        if ((address & 3U) || address >= ram.size() * 4U) return false;
        ram[address / 4U] = value; return true;
    };

    constexpr std::uint32_t addiu8 = (0x09U << 26U) | (8U << 16U) | 8U;
    constexpr std::uint32_t addiu9 = (0x09U << 26U) | (9U << 16U) | 9U;
    constexpr std::uint32_t daddu = (8U << 21U) | (9U << 16U) | (10U << 11U) | 0x2DU;
    check(cpu.step(addiu8) && cpu.gpr(8).lo == 8, "EE ADDIU executes");
    check(cpu.step(addiu9) && cpu.gpr(9).lo == 9, "EE ADDIU writes second register");
    check(cpu.step(daddu) && cpu.gpr(10).lo == 17, "EE DADDU adds 64-bit register values");
    check(cpu.gpr(0).lo == 0 && cpu.gpr(0).hi == 0, "EE register zero is immutable");
    check(cpu.pc() == 12, "EE sequential PC advances by four bytes");

    cpu.reset();
    constexpr std::uint32_t lui = (0x0FU << 26U) | (15U << 16U) | 0x8000U;
    check(cpu.step(lui) && cpu.gpr(15).lo == 0xFFFFFFFF80000000ULL,
          "LUI sign-extends its 32-bit result");
    cpu.reset();
    cpu.set_gpr(8, {0xFFFFFFFFULL, 0});
    constexpr std::uint32_t addu = (8U << 21U) | (8U << 16U) | (11U << 11U) | 0x21U;
    check(cpu.step(addu) && cpu.gpr(11).lo == 0xFFFFFFFFFFFFFFFEULL, "ADDU wraps at 32 bits and sign-extends result");
    cpu.reset();
    cpu.set_gpr(8, {0x7FFFFFFFULL, 0});
    constexpr std::uint32_t addiu_wrap = (0x09U << 26U) | (8U << 21U) | (14U << 16U) | 1U;
    check(cpu.step(addiu_wrap) && cpu.gpr(14).lo == 0xFFFFFFFF80000000ULL,
          "ADDIU wraps to 32 bits then sign-extends the result");
    cpu.reset();
    cpu.set_gpr(8, {0x100000000ULL, 0});
    cpu.set_gpr(9, {1, 0});
    constexpr std::uint32_t dsubu = (8U << 21U) | (9U << 16U) | (12U << 11U) | 0x2FU;
    check(cpu.step(dsubu) && cpu.gpr(12).lo == 0xFFFFFFFFULL, "DSUBU subtracts full 64-bit values");
    cpu.reset();
    cpu.set_gpr(8, {10, 0});
    constexpr std::uint32_t daddiu = (0x19U << 26U) | (8U << 21U) | (13U << 16U) | 5U;
    check(cpu.step(daddiu) && cpu.gpr(13).lo == 15, "DADDIU adds signed immediate to 64-bit value");

    cpu.reset();
    cpu.set_gpr(8, {10, 0});
    constexpr std::uint32_t addi = (0x08U << 26U) | (8U << 21U) | (13U << 16U) | 5U;
    check(cpu.step(addi) && cpu.gpr(13).lo == 15, "ADDI adds signed immediate");

    cpu.reset();
    cpu.set_gpr(8, {0xFFFFFFFFFFFFFFFEULL, 0});
    cpu.set_gpr(9, {1, 0});
    constexpr std::uint32_t slt = (8U << 21U) | (9U << 16U) | (11U << 11U) | 0x2AU;
    constexpr std::uint32_t sltu = (8U << 21U) | (9U << 16U) | (12U << 11U) | 0x2BU;
    check(cpu.step(slt) && cpu.gpr(11).lo == 1, "EE SLT compares signed values");
    check(cpu.step(sltu) && cpu.gpr(12).lo == 0, "EE SLTU compares unsigned values");

    cpu.reset();
    cpu.set_gpr(8, {0xFFFFFFFFFFFFFFFEULL, 0});
    constexpr std::uint32_t slti_negative = (0x0AU << 26U) | (8U << 21U) | (11U << 16U) | 0xFFFFU;
    check(cpu.step(slti_negative) && cpu.gpr(11).lo == 1,
          "SLTI sign-extends immediate and compares signed values");
    cpu.reset();
    cpu.set_gpr(8, {0xFFFFFFFFFFFFFFFEULL, 0});
    constexpr std::uint32_t sltiu_negative = (0x0BU << 26U) | (8U << 21U) | (12U << 16U) | 0xFFFFU;
    check(cpu.step(sltiu_negative) && cpu.gpr(12).lo == 1,
          "SLTIU compares unsigned values against sign-extended immediate");

    cpu.reset();
    constexpr std::uint32_t beq = (0x04U << 26U) | (8U << 21U) | (9U << 16U) | 2U;
    cpu.set_gpr(8, {4, 0}); cpu.set_gpr(9, {4, 0});
    check(cpu.step(beq), "BEQ schedules branch");
    check(cpu.pc() == 4 && cpu.branch_pending(), "branch executes one delay-slot instruction");
    check(cpu.step(addiu8), "delay-slot instruction executes");
    check(cpu.pc() == 12 && !cpu.branch_pending(), "taken BEQ applies target after delay slot");

    cpu.reset();
    cpu.set_gpr(8, {static_cast<std::uint64_t>(-1LL), 0});
    constexpr std::uint32_t bltz = (0x01U << 26U) | (8U << 21U) | (0U << 16U) | 2U;
    check(cpu.step(bltz) && cpu.branch_pending(), "BLTZ schedules branch for negative register");
    check(cpu.step(0) && cpu.pc() == 12, "BLTZ applies target after delay slot");
    cpu.reset();
    cpu.set_gpr(8, {0, 0});
    constexpr std::uint32_t bgezal = (0x01U << 26U) | (8U << 21U) | (0x11U << 16U) | 1U;
    check(cpu.step(bgezal) && cpu.gpr(31).lo == 8 && cpu.branch_pending(),
          "BGEZAL links return address and schedules branch when condition is true");
    check(cpu.step(0) && cpu.pc() == 8, "BGEZAL applies target after delay slot");
    cpu.reset();
    cpu.set_gpr(8, {static_cast<std::uint64_t>(-1LL), 0});
    constexpr std::uint32_t bgez = (0x01U << 26U) | (8U << 21U) | (1U << 16U) | 1U;
    check(cpu.step(bgez) && cpu.branch_pending(), "BGEZ schedules not-taken path");
    check(cpu.step(0) && cpu.pc() == 8, "BGEZ not-taken path skips branch target");

    cpu.reset();
    cpu.set_gpr(8, {0, 0});
    constexpr std::uint32_t blez = (0x06U << 26U) | (8U << 21U) | 2U;
    check(cpu.step(blez), "BLEZ schedules branch for zero value");
    check(cpu.branch_pending(), "BLEZ branch is pending before delay slot");
    check(cpu.step(0), "BLEZ delay-slot instruction executes");
    check(cpu.pc() == 12, "BLEZ applies branch target after delay slot");

    cpu.reset();
    cpu.set_gpr(8, {5, 0});
    constexpr std::uint32_t bgtz = (0x07U << 26U) | (8U << 21U) | 1U;
    check(cpu.step(bgtz), "BGTZ schedules branch for positive value");
    check(cpu.step(0), "BGTZ delay-slot instruction executes");
    check(cpu.pc() == 8, "BGTZ applies branch target after delay slot");

    cpu.reset();
    cpu.set_pc(0x1000);
    cpu.set_gpr(8, {0x3450, 0});
    constexpr std::uint32_t jalr = (8U << 21U) | (9U << 11U) | 0x09U;
    check(cpu.step(jalr) && cpu.gpr(9).lo == 0x1008 && cpu.branch_pending(),
          "JALR stores return address in selected register and schedules target");
    check(cpu.step(0) && cpu.pc() == 0x3450, "JALR applies register target after delay slot");

    cpu.reset();
    cpu.set_pc(0x1000);
    constexpr std::uint32_t jump = (0x02U << 26U) | 0x00000020U;
    check(cpu.step(jump) && cpu.branch_pending(), "J schedules absolute pseudo-direct target");
    check(cpu.step(0) && cpu.pc() == 0x80, "J applies target after delay slot");

    cpu.reset();
    cpu.set_pc(0x1000);
    constexpr std::uint32_t jal = (0x03U << 26U) | 0x00000030U;
    check(cpu.step(jal) && cpu.gpr(31).lo == 0x1008, "JAL saves return address after delay slot");
    check(cpu.step(0) && cpu.pc() == 0xC0, "JAL applies target after delay slot");

    cpu.reset();
    cpu.set_pc(0x80000000ULL);
    constexpr std::uint32_t ori = (0x0DU << 26U) | (8U << 21U) | (9U << 16U) | 0x00F0U;
    cpu.set_gpr(8, {0x1000, 0});
    check(cpu.step(ori) && cpu.gpr(9).lo == 0x10F0, "ORI zero-extends immediate and combines bits");

    cpu.reset();
    cpu.set_gpr(8, {0x10, 0});
    cpu.set_gpr(9, {0x12345678, 0});
    constexpr std::uint32_t sw = (0x2BU << 26U) | (8U << 21U) | (9U << 16U);
    check(cpu.step(sw, nullptr, read_word, write_word), "SW stores register to emulated RAM");
    check(ram[4] == 0x12345678, "SW stores expected little-endian word");
    cpu.reset();
    cpu.set_gpr(8, {0x10, 0});
    constexpr std::uint32_t lw = (0x23U << 26U) | (8U << 21U) | (10U << 16U);
    check(cpu.step(lw, nullptr, read_word, write_word) && cpu.gpr(10).lo == 0x12345678, "LW loads word from emulated RAM");
    cpu.reset();
    cpu.set_gpr(8, {0x10, 0});
    cpu.set_gpr(9, {0xAA, 0});
    constexpr std::uint32_t sb = (0x28U << 26U) | (8U << 21U) | (9U << 16U) | 1U;
    check(cpu.step(sb, nullptr, read_word, write_word), "SB stores one byte");
    check(ram[4] == 0x1234AA78, "SB preserves adjacent bytes");
    cpu.reset();
    cpu.set_gpr(8, {0x10, 0});
    constexpr std::uint32_t lb = (0x20U << 26U) | (8U << 21U) | (10U << 16U) | 1U;
    check(cpu.step(lb, nullptr, read_word, write_word) && cpu.gpr(10).lo == 0xFFFFFFFFFFFFFFAAULL, "LB sign-extends byte");
    cpu.reset();
    ram[4] = 0x44332211U;
    cpu.set_gpr(8, {0x10, 0}); cpu.set_gpr(10, {0xAABBCCDDU, 0});
    constexpr std::uint32_t lwl = (0x22U << 26U) | (8U << 21U) | (10U << 16U) | 1U;
    check(cpu.step(lwl, nullptr, read_word, write_word) && cpu.gpr(10).lo == 0x2211CCDDU,
          "LWL merges addressed bytes into the register without clobbering preserved bytes");
    cpu.reset();
    ram[4] = 0x44332211U;
    cpu.set_gpr(8, {0x10, 0}); cpu.set_gpr(10, {0xAABBCCDDU, 0});
    constexpr std::uint32_t lwr = (0x26U << 26U) | (8U << 21U) | (10U << 16U) | 1U;
    check(cpu.step(lwr, nullptr, read_word, write_word) && cpu.gpr(10).lo == 0xFFFFFFFFAA443322ULL,
          "LWR merges addressed bytes into the register without clobbering preserved bytes");
    cpu.reset();
    ram[4] = 0x80FF7F01U;
    cpu.set_gpr(8, {0x10, 0});
    constexpr std::uint32_t lh = (0x21U << 26U) | (8U << 21U) | (10U << 16U) | 2U;
    check(cpu.step(lh, nullptr, read_word, write_word) &&
          cpu.gpr(10).lo == 0xFFFFFFFFFFFF80FFULL, "LH sign-extends negative halfword");
    cpu.reset();
    cpu.set_gpr(8, {0x10, 0});
    constexpr std::uint32_t lhu = (0x25U << 26U) | (8U << 21U) | (10U << 16U) | 2U;
    check(cpu.step(lhu, nullptr, read_word, write_word) && cpu.gpr(10).lo == 0x80FFU,
          "LHU zero-extends halfword");
    cpu.reset();
    cpu.set_gpr(8, {0x10, 0});
    cpu.set_gpr(9, {0xABCD, 0});
    constexpr std::uint32_t sh = (0x29U << 26U) | (8U << 21U) | (9U << 16U) | 2U;
    check(cpu.step(sh, nullptr, read_word, write_word), "SH stores halfword");
    check(ram[4] == 0xABCD7F01U, "SH preserves adjacent halfword");

    cpu.reset();
    std::string memory_error;
    cpu.set_gpr(8, {0x200, 0});
    check(!cpu.step(lw, &memory_error, read_word, write_word), "out-of-range LW is rejected");
    check(memory_error.find("memory access failed") != std::string::npos, "memory fault reports diagnostic");

    cpu.reset();
    cpu.set_gpr(8, {0x123456789ABCDEF0ULL, 0});
    constexpr std::uint32_t dsll32 = (8U << 16U) | (13U << 11U) | (1U << 6U) | 0x3CU;
    check(cpu.step(dsll32) && cpu.gpr(13).lo == 0x3579BDE000000000ULL, "DSLL32 shifts doubleword by shamt plus 32");

    cpu.reset();
    cpu.set_gpr(8, {0x10, 0});
    cpu.set_gpr(9, {0x1122334455667788ULL, 0x99AABBCCDDEEFF00ULL});
    constexpr std::uint32_t sq = (0x1FU << 26U) | (8U << 21U) | (9U << 16U);
    check(cpu.step(sq, nullptr, read_word, write_word), "SQ stores 128-bit register to emulated RAM");
    check(ram[4] == 0x55667788 && ram[5] == 0x11223344 && ram[6] == 0xDDEEFF00 && ram[7] == 0x99AABBCC,
          "SQ writes all four 32-bit words in little-endian order");
    cpu.reset();
    cpu.set_gpr(8, {0x10, 0});
    constexpr std::uint32_t lq = (0x1EU << 26U) | (8U << 21U) | (10U << 16U);
    check(cpu.step(lq, nullptr, read_word, write_word), "LQ loads 128-bit value from emulated RAM");
    check(cpu.gpr(10).lo == 0x1122334455667788ULL && cpu.gpr(10).hi == 0x99AABBCCDDEEFF00ULL,
          "LQ reconstructs low and high 64-bit halves");

    cpu.reset();
    cpu.set_gpr(8, {0x10, 0});
    cpu.set_gpr(9, {0x0123456789ABCDEFULL, 0});
    constexpr std::uint32_t sd = (0x3FU << 26U) | (8U << 21U) | (9U << 16U);
    check(cpu.step(sd, nullptr, read_word, write_word), "SD stores 64-bit doubleword");
    check(ram[4] == 0x89ABCDEFU && ram[5] == 0x01234567U, "SD writes both words in little-endian order");
    cpu.reset();
    cpu.set_gpr(8, {0x10, 0});
    constexpr std::uint32_t ld = (0x37U << 26U) | (8U << 21U) | (10U << 16U);
    check(cpu.step(ld, nullptr, read_word, write_word) && cpu.gpr(10).lo == 0x0123456789ABCDEFULL,
          "LD reconstructs 64-bit doubleword");

    cpu.reset();
    cpu.set_gpr(8, {7, 0});
    cpu.set_gpr(9, {6, 0});
    constexpr std::uint32_t mult = (8U << 21U) | (9U << 16U) | 0x18U;
    check(cpu.step(mult) && cpu.lo() == 42 && cpu.hi() == 0, "MULT computes signed product into HI/LO");
    constexpr std::uint32_t mflo = (10U << 11U) | 0x12U;
    check(cpu.step(mflo) && cpu.gpr(10).lo == 42, "MFLO reads the low product");
    cpu.reset();
    cpu.set_gpr(8, {static_cast<std::uint64_t>(static_cast<std::int64_t>(-42)), 0});
    cpu.set_gpr(9, {5, 0});
    constexpr std::uint32_t div = (8U << 21U) | (9U << 16U) | 0x1AU;
    check(cpu.step(div) && static_cast<std::int32_t>(cpu.lo()) == -8 &&
          static_cast<std::int32_t>(cpu.hi()) == -2, "DIV stores signed quotient and remainder");
    constexpr std::uint32_t mfhi = (11U << 11U) | 0x10U;
    check(cpu.step(mfhi) && static_cast<std::int32_t>(cpu.gpr(11).lo) == -2, "MFHI reads signed division remainder");
    cpu.reset();
    cpu.set_gpr(8, {0xFFFFFFFFULL, 0});
    cpu.set_gpr(9, {2, 0});
    constexpr std::uint32_t multu = (8U << 21U) | (9U << 16U) | 0x19U;
    check(cpu.step(multu) && cpu.lo() == 0xFFFFFFFEULL && cpu.hi() == 1,
          "MULTU preserves unsigned 32-bit product halves");

    cpu.reset();
    cpu.set_gpr(8, {0x100000000ULL, 0xABCDEFULL});
    cpu.set_gpr(9, {0, 0});
    constexpr std::uint32_t movz = (8U << 21U) | (9U << 16U) | (10U << 11U) | 0x0AU;
    check(cpu.step(movz) && cpu.gpr(10).lo == 0x100000000ULL,
          "MOVZ copies source when second register is zero");
    cpu.reset();
    cpu.set_gpr(8, {11, 0}); cpu.set_gpr(9, {1, 0}); cpu.set_gpr(10, {99, 0});
    constexpr std::uint32_t movn = (8U << 21U) | (9U << 16U) | (10U << 11U) | 0x0BU;
    check(cpu.step(movn) && cpu.gpr(10).lo == 11, "MOVN copies source when second register is nonzero");
    cpu.reset();
    cpu.set_gpr(8, {0x100000000ULL, 0}); cpu.set_gpr(9, {7, 0});
    constexpr std::uint32_t dadd = (8U << 21U) | (9U << 16U) | (10U << 11U) | 0x2CU;
    check(cpu.step(dadd) && cpu.gpr(10).lo == 0x100000007ULL, "DADD adds full 64-bit values");
    cpu.reset();
    cpu.set_gpr(8, {0x100000007ULL, 0}); cpu.set_gpr(9, {7, 0});
    constexpr std::uint32_t dsub = (8U << 21U) | (9U << 16U) | (10U << 11U) | 0x2EU;
    check(cpu.step(dsub) && cpu.gpr(10).lo == 0x100000000ULL, "DSUB subtracts full 64-bit values");

    cpu.reset();
    cpu.set_gpr(8, {0xFFFFFFFF80000001ULL, 0});
    constexpr std::uint32_t mtc0_status = (0x10U << 26U) | (0x04U << 21U) | (8U << 16U) | (12U << 11U);
    check(cpu.step(mtc0_status) && cpu.cp0(12) == 0x80000001U,
          "MTC0 writes the selected 32-bit privileged register");
    constexpr std::uint32_t mfc0_status = (0x10U << 26U) | (9U << 16U) | (12U << 11U);
    check(cpu.step(mfc0_status) && cpu.gpr(9).lo == 0xFFFFFFFF80000001ULL,
          "MFC0 sign-extends the selected privileged register into a GPR");
    cpu.reset();
    check(cpu.cp0(12) == 0, "CPU reset clears privileged registers");
    check(cpu.cp0(32) == 0, "out-of-range privileged-register reads are safe");

    cpu.reset();
    std::string error;
    check(!cpu.step(0xF8000000U, &error), "unsupported EE instruction is rejected");
    check(error.find("unsupported") != std::string::npos, "unsupported instruction has diagnostic");
    cpu.reset(); cpu.set_gpr(0, {99, 99});
    check(cpu.gpr(0).lo == 0 && cpu.gpr(0).hi == 0, "writes cannot alter EE zero register");
    return failures == 0 ? 0 : 1;
}
