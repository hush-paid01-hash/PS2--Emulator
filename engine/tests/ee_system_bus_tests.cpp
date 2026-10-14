#include "retroshelf/ee_system_bus.hpp"
#include <iostream>
int main() {
    retroshelf::ee::EeBus ram;
    retroshelf::ee::EeHardware hardware;
    retroshelf::ee::EeSystemBus bus(ram, hardware);
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << '\n'; ++failures; }
        else { std::cout << "PASS: " << label << '\n'; }
    };
    std::uint32_t value = 0;
    check(bus.write32(0x100, 0x12345678U), "write to RAM routes to memory");
    check(bus.read32(0x100, value) && value == 0x12345678U, "read from RAM returns stored word");
    check(bus.write32(retroshelf::ee::EeHardware::Timer0Target, 77), "write timer target routes to hardware");
    check(bus.read32(retroshelf::ee::EeHardware::Timer0Target, value) && value == 77,
          "timer register reads from hardware device");
    check(bus.write8(retroshelf::ee::EeHardware::InterruptMask, 1), "byte write routes to MMIO");
    check(bus.read32(retroshelf::ee::EeHardware::InterruptMask, value) && value == 1,
          "MMIO byte write updates addressed lane");
    check(!bus.read32(0x10000040U, value), "unmapped MMIO address is rejected");
    check(!bus.read32(0xFFFFFFFCU, value), "invalid RAM address is rejected");
    return failures == 0 ? 0 : 1;
}
