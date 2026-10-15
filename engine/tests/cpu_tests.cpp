#include "retroshelf/cpu.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "retroshelf/system_bus.hpp"

int main() {
    std::vector<std::uint8_t> ram(64 * 1024, 0);
    retroshelf::SystemBus bus(ram);
    retroshelf::Cpu cpu(bus);
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
    check(cpu.step(sw), "SW stores a word through the shared bus");
    check(cpu.step(lw), "LW loads a word through the shared bus");
    check(cpu.reg(13) == 0x12345678, "LW returns the stored word");

    cpu.set_reg(11, 3);
    std::string error;
    check(!cpu.step(lw, &error), "misaligned LW is rejected");
    check(!error.empty(), "invalid memory access explains why");

    cpu.set_reg(11, ram.size());
    error.clear();
    check(!cpu.step(sw, &error), "out-of-range SW is rejected");

    check(ram[16] == 0x78 && ram[19] == 0x12, "SW wrote little-endian bytes into shared RAM");
    cpu.set_reg(11, ram.size() - 4);
    check(cpu.step(sw), "SW at last aligned word succeeds");
    cpu.set_reg(11, 0x100000000ULL);
    error.clear();
    check(!cpu.step(lw, &error) && !error.empty(), "address beyond 32 bits is rejected");
    cpu.set_reg(11, ram.size() + 4);
    check(!cpu.step(lw, &error), "out-of-range LW is rejected");

    ram[32] = 0xEF; ram[33] = 0xBE; ram[34] = 0xAD; ram[35] = 0xDE;
    cpu.set_reg(11, 32);
    check(cpu.step(lw), "LW reads bytes written via the bus");
    check(cpu.reg(13) == 0xFFFFFFFFDEADBEEFULL, "LW observes shared-bus write and sign-extends");
    std::vector<std::uint8_t> bytes;
    check(bus.read(16, 4, bytes) && bytes[0] == 0x78, "bus observes CPU write");

    cpu.reset();
    check(cpu.reg(11) == 0 && cpu.pc() == 0, "reset clears registers and PC");
    check(ram[16] == 0x78, "reset does not clear shared RAM");
    error.clear();
    check(!cpu.step(0xFFFFFFFFU, &error), "unsupported instruction is rejected");
    check(!error.empty(), "unsupported instruction explains why");
    return failures == 0 ? 0 : 1;
}
