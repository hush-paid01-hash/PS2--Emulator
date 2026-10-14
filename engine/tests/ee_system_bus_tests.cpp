#include "retroshelf/ee_system_bus.hpp"
#include <cstdint>
#include <string>
#include <vector>
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
    check(!bus.read32(retroshelf::ee::EeSystemBus::BiosBase, value),
          "BIOS address faults when no ROM is loaded");
    const std::vector<std::uint8_t> bios{0x78, 0x56, 0x34, 0x12, 0xAA};
    std::string error;
    check(bus.load_bios(bios, &error) && bus.bios_loaded(), "valid BIOS bytes load into ROM window");
    check(bus.read32(retroshelf::ee::EeSystemBus::BiosBase, value) && value == 0x12345678U,
          "BIOS ROM reads little-endian instruction words");
    check(bus.read32(0xBFC00000U, value) && value == 0x12345678U,
          "BIOS ROM is visible through the EE reset-vector alias");
    std::uint8_t byte = 0;
    check(bus.read8(retroshelf::ee::EeSystemBus::BiosBase + 4U, byte) && byte == 0xAA,
          "BIOS ROM supports byte reads inside image");
    check(!bus.read8(retroshelf::ee::EeSystemBus::BiosBase + 8U, byte),
          "BIOS ROM does not expose bytes beyond loaded image");
    check(!bus.write32(retroshelf::ee::EeSystemBus::BiosBase, 0),
          "CPU writes to BIOS ROM are rejected");
    check(!bus.write32(0xBFC00000U, 0),
          "CPU writes through BIOS reset-vector alias are rejected");
    check(bus.read32(retroshelf::ee::EeSystemBus::BiosBase, value) && value == 0x12345678U,
          "rejected ROM write leaves BIOS content unchanged");
    const std::vector<std::uint8_t> invalid_bios;
    check(!bus.load_bios(invalid_bios, &error) && bus.bios_loaded(),
          "invalid BIOS replacement is rejected without discarding loaded BIOS");
    check(!bus.read32(0xFFFFFFFCU, value), "invalid RAM address is rejected");
    return failures == 0 ? 0 : 1;
}
