#include "retroshelf/cpu.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
void put32(std::vector<std::uint8_t>& ram, std::size_t offset, std::uint32_t value) {
    ram[offset] = static_cast<std::uint8_t>(value);
    ram[offset + 1] = static_cast<std::uint8_t>(value >> 8U);
    ram[offset + 2] = static_cast<std::uint8_t>(value >> 16U);
    ram[offset + 3] = static_cast<std::uint8_t>(value >> 24U);
}
}

int main() {
    retroshelf::Cpu cpu;
    std::vector<std::uint8_t> ram(8, 0);
    // addiu $t0, $zero, 7; addiu $t1, $zero, 5
    put32(ram, 0, (0x09U << 26U) | (8U << 16U) | 7U);
    put32(ram, 4, (0x09U << 26U) | (9U << 16U) | 5U);
    std::string error;

    assert(cpu.step_from_ram(ram, 0, &error));
    assert(cpu.reg(8) == 7);
    assert(cpu.pc() == 4);
    assert(cpu.step_from_ram(ram, 0, &error));
    assert(cpu.reg(9) == 5);
    assert(cpu.pc() == 8);

    // Failed fetch must not advance PC or alter registers.
    const auto pc_before = cpu.pc();
    assert(!cpu.step_from_ram(ram, 0, &error));
    assert(cpu.pc() == pc_before);
    assert(!error.empty());

    cpu.set_pc(0x1000);
    assert(!cpu.step_from_ram(ram, 0, &error));
    assert(cpu.pc() == 0x1000);

    cpu.set_pc(0);
    assert(!cpu.step_from_ram(ram, 4, &error));
    assert(cpu.pc() == 0);

    // LW/SW through step_from_ram use caller-owned RAM for data accesses too.
    std::vector<std::uint8_t> data_ram(64, 0);
    put32(data_ram, 0, (0x0FU << 26U) | (8U << 16U) | 0x0010U); // lui $t0, 0x0010
    put32(data_ram, 4, (0x09U << 26U) | (9U << 16U) | 0x1234U); // addiu $t1, $zero, 0x1234
    put32(data_ram, 8, (0x2BU << 26U) | (8U << 21U) | (9U << 16U) | 0x20U); // sw $t1, 0x20($t0)
    put32(data_ram, 12, (0x23U << 26U) | (8U << 21U) | (10U << 16U) | 0x20U); // lw $t2, 0x20($t0)
    cpu.reset();
    cpu.set_pc(0x00100000);
    for (int i = 0; i < 4; ++i) assert(cpu.step_from_ram(data_ram, 0x00100000, &error));
    assert(data_ram[0x20] == 0x34 && data_ram[0x21] == 0x12);
    assert(data_ram[0x22] == 0x00 && data_ram[0x23] == 0x00);
    assert(cpu.reg(10) == 0x1234);
    std::cout << "CPU RAM fetch/data-access tests passed\n";
}
