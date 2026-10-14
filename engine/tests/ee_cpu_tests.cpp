#include "retroshelf/ee_cpu.hpp"

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

    constexpr std::uint32_t addiu8 = (0x09U << 26U) | (8U << 16U) | 8U;
    constexpr std::uint32_t addiu9 = (0x09U << 26U) | (9U << 16U) | 9U;
    constexpr std::uint32_t daddu = (8U << 21U) | (9U << 16U) | (10U << 11U) | 0x2DU;
    check(cpu.step(addiu8) && cpu.gpr(8).lo == 8, "EE ADDIU executes");
    check(cpu.step(addiu9) && cpu.gpr(9).lo == 9, "EE ADDIU writes second register");
    check(cpu.step(daddu) && cpu.gpr(10).lo == 17, "EE DADDU adds 64-bit register values");
    check(cpu.gpr(0).lo == 0 && cpu.gpr(0).hi == 0, "EE register zero is immutable");
    check(cpu.pc() == 12, "EE sequential PC advances by four bytes");

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
    constexpr std::uint32_t beq = (0x04U << 26U) | (8U << 21U) | (9U << 16U) | 2U;
    cpu.set_gpr(8, {4, 0}); cpu.set_gpr(9, {4, 0});
    check(cpu.step(beq), "BEQ schedules branch");
    check(cpu.pc() == 4 && cpu.branch_pending(), "branch executes one delay-slot instruction");
    check(cpu.step(addiu8), "delay-slot instruction executes");
    check(cpu.pc() == 12 && !cpu.branch_pending(), "taken BEQ applies target after delay slot");

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
    std::string error;
    check(!cpu.step(0xFFFFFFFFU, &error), "unsupported EE instruction is rejected");
    check(error.find("unsupported") != std::string::npos, "unsupported instruction has diagnostic");
    cpu.reset(); cpu.set_gpr(0, {99, 99});
    check(cpu.gpr(0).lo == 0 && cpu.gpr(0).hi == 0, "writes cannot alter EE zero register");
    return failures == 0 ? 0 : 1;
}
