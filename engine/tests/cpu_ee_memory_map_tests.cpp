#include "retroshelf/cpu.hpp"
#include "retroshelf/ps2_ee_memory_map.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

int main() {
    retroshelf::Ps2EeMemoryMap memory(1024U * 1024U);
    retroshelf::Cpu cpu;
    std::string error;

    // SW $t1, 0($t0); LW $t2, 0($t0), fetched via the sign-extended KSEG0 alias.
    assert(memory.write(0x80000100U, 4,
                        (0x2BU << 26U) | (8U << 21U) | (9U << 16U), &error));
    assert(memory.write(0x80000104U, 4,
                        (0x23U << 26U) | (8U << 21U) | (10U << 16U), &error));

    cpu.set_pc(0xFFFFFFFF80000100ULL);
    cpu.set_reg(8, 0xFFFFFFFFA0000200ULL); // KSEG1 data alias
    cpu.set_reg(9, 0x12345678U);

    assert(cpu.step_from_ee_memory(memory, &error));
    assert(cpu.pc() == 0xFFFFFFFF80000104ULL);
    std::uint64_t value = 0;
    assert(memory.read(0x200U, 4, value, &error) && value == 0x12345678U);

    assert(cpu.step_from_ee_memory(memory, &error));
    assert(cpu.reg(10) == 0x12345678U);

    // A misaligned EE store must raise the modeled store-address exception.
    cpu.reset();
    cpu.set_pc(0xFFFFFFFF80000100ULL);
    cpu.set_reg(8, 0xFFFFFFFF80000201ULL);
    cpu.set_reg(9, 0xAABBCCDDU);
    assert(!cpu.step_from_ee_memory(memory, &error));
    assert((cpu.cop0_cause() & (0x1FU << 2U)) ==
           (retroshelf::Cpu::ExcAddressErrorStore << 2U));

    // An unmapped instruction fetch fails without advancing the PC.
    cpu.reset();
    cpu.set_pc(0xFFFFFFFFB0000000ULL);
    const auto pc_before = cpu.pc();
    assert(!cpu.step_from_ee_memory(memory, &error));
    assert(cpu.pc() == pc_before);
    assert(!error.empty());

    std::cout << "CPU EE memory-map integration tests passed\\n";
    return 0;
}
