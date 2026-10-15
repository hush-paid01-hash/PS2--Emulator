#include "retroshelf/cpu.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

#include "retroshelf/system_bus.hpp"

int main() {
    std::vector<std::uint8_t> ee_ram(64 * 1024, 0);
    retroshelf::SystemBus bus(ee_ram);
    retroshelf::Cpu cpu(bus);

    // addiu $t0, $zero, 7
    constexpr std::uint32_t load_seven = (0x09U << 26U) | (8U << 16U) | 7U;
    // addiu $t1, $zero, 5
    constexpr std::uint32_t load_five = (0x09U << 26U) | (9U << 16U) | 5U;
    // addu $t2, $t0, $t1
    constexpr std::uint32_t add = (8U << 21U) | (9U << 16U) | (10U << 11U) | 0x21U;

    if (!cpu.step(load_seven) || !cpu.step(load_five) || !cpu.step(add)) {
        std::cerr << "CPU demo failed to execute an instruction.\n";
        return 1;
    }

    std::cout << "RetroShelf engine foundation\n";
    std::cout << "Test calculation: 7 + 5 = " << cpu.reg(10) << "\n";
    std::cout << "Instructions tested: 3\n";
    std::cout << "Note: this is a starter CPU subset, not a playable PS2 emulator yet.\n";
    return cpu.reg(10) == 12 ? 0 : 1;
}
