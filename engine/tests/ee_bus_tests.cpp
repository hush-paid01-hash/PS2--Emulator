#include "retroshelf/ee_bus.hpp"
#include <cstdint>
#include <iostream>

int main() {
    retroshelf::ee::EeBus bus;
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << '\n'; ++failures; }
        else { std::cout << "PASS: " << label << '\n'; }
    };
    std::uint32_t word = 0;
    check(bus.ram_size() == 32U * 1024U * 1024U, "32 MiB EE main RAM is allocated");
    check(bus.write32(0x100, 0x12345678U), "aligned word write succeeds");
    check(bus.read32(0x100, word) && word == 0x12345678U, "word read preserves little-endian value");
    std::uint8_t byte = 0;
    check(bus.read8(0x100, byte) && byte == 0x78, "byte read sees low byte");
    check(!bus.read32(0x101, word), "unaligned word read is rejected");
    check(!bus.write32(static_cast<std::uint32_t>(bus.ram_size() - 2), 0), "out-of-range word write is rejected");
    check(!bus.read8(static_cast<std::uint32_t>(bus.ram_size()), byte), "out-of-range byte read is rejected");
    check(bus.write32(0x80000100U, 0xA1B2C3D4U), "cached KSEG0 RAM alias writes successfully");
    check(bus.read32(0x00000100U, word) && word == 0xA1B2C3D4U, "cached alias maps to physical RAM");
    check(bus.read32(0xA0000100U, word) && word == 0xA1B2C3D4U, "uncached KSEG1 alias reads same RAM");
    std::uint8_t alias_byte = 0;
    check(!bus.read8(0x82000000U, alias_byte), "address outside mapped 32 MiB RAM is rejected");
    bus.reset();
    check(bus.read32(0x100, word) && word == 0, "reset clears physical RAM");
    check(bus.read32(0x80000100U, word) && word == 0, "reset clears cached alias RAM");
    return failures == 0 ? 0 : 1;
}
