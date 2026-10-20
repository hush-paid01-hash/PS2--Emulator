#include "retroshelf/ps2_ee_memory_map.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

int main() {
    retroshelf::Ps2EeMemoryMap memory(1024U * 1024U);
    int failures = 0;
    const auto check = [&failures](bool condition, const char* label) {
        if (!condition) { std::cerr << "FAIL: " << label << '\n'; ++failures; }
        else std::cout << "PASS: " << label << '\n';
    };
    std::string error;
    std::uint64_t value = 0;

    check(memory.write(0x100, 4, 0x12345678, &error), "write little-endian word to RAM");
    check(memory.read(0x100, 4, value, &error) && value == 0x12345678,
          "read word from physical RAM");
    check(memory.read(0x20000100, 4, value, &error) && value == 0x12345678,
          "main RAM mirror resolves to same bytes");
    check(memory.read(0x80000100, 4, value, &error) && value == 0x12345678,
          "KSEG0 cached alias resolves to RAM");
    check(memory.read(0xA0000100, 4, value, &error) && value == 0x12345678,
          "KSEG1 uncached alias resolves to RAM");

    check(memory.write(0x104, 1, 0xFE, &error), "byte write succeeds");
    check(memory.read(0x104, 1, value, &error) && value == 0xFE,
          "byte read returns unsigned byte value");
    check(!memory.read(0x101, 4, value, &error) && !error.empty(),
          "unaligned word access is rejected");
    check(!memory.write(0x40000000, 4, 1, &error) && !error.empty(),
          "unmapped address is rejected");
    check(!memory.write(0x100000, 8, 1, &error) && !error.empty(),
          "out-of-range access is rejected for small configured RAM");

    retroshelf::Ps2EeMemoryMap small(256);
    check(!small.read(0x100, 8, value, &error), "access beyond configured RAM is rejected");
    // BIOS bytes are mapped read-only through physical, KSEG0 and KSEG1 aliases.
    std::vector<std::uint8_t> bios(16);
    for (std::size_t i = 0; i < bios.size(); ++i)
        bios[i] = static_cast<std::uint8_t>(0x10U + i);
    check(memory.load_bios_image(bios, &error), "load synthetic BIOS ROM image");
    check(memory.bios_rom_size() == bios.size(), "BIOS ROM reports mapped image size");
    check(memory.read(retroshelf::Ps2EeMemoryMap::BiosRomBase, 4, value, &error) &&
          value == 0x13121110U, "read BIOS bytes from physical ROM window");
    check(memory.read(0x9FC00000U, 4, value, &error) && value == 0x13121110U,
          "read BIOS bytes through KSEG0 alias");
    check(memory.read(0xBFC00000U, 4, value, &error) && value == 0x13121110U,
          "read BIOS bytes through KSEG1 reset-vector alias");
    check(!memory.write(0xBFC00000U, 4, 0xDEADBEEFU, &error) && !error.empty(),
          "writes to BIOS ROM are rejected");
    check(memory.read(0x1FC00000U, 4, value, &error) && value == 0x13121110U,
          "rejected BIOS write leaves ROM contents unchanged");
    std::uint64_t low = 0, high = 0;
    check(memory.read128(0xBFC00000U, low, high, &error) &&
          low == 0x1716151413121110ULL && high == 0x1F1E1D1C1B1A1918ULL,
          "read aligned 128-bit BIOS data");
    check(!memory.write128(0xBFC00000U, 0, 0, &error) && !error.empty(),
          "128-bit BIOS writes are rejected");
    check(!memory.load_bios_image({}, &error), "empty BIOS image is rejected");
    check(!memory.load_bios_image(std::vector<std::uint8_t>(
              retroshelf::Ps2EeMemoryMap::BiosRomWindowSize + 1U, 0), &error),
          "BIOS image larger than ROM window is rejected");

    return failures == 0 ? 0 : 1;
}
