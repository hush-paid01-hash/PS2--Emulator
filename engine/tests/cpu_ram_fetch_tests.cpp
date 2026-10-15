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

    cpu.set_pc(2);
    assert(!cpu.step_from_ram(ram, 0, &error));
    assert(cpu.pc() == 2);
    assert(!error.empty());

    cpu.set_pc(0);
    const std::vector<std::uint8_t> short_ram(3, 0);
    assert(!cpu.step_from_ram(short_ram, 0, &error));
    assert(cpu.pc() == 0);
    assert(!error.empty());

    std::cout << "CPU RAM fetch tests passed\n";
}
