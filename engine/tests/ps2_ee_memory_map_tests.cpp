#include "retroshelf/ps2_ee_memory_map.hpp"

#include <cstdint>
#include <iostream>
#include <string>

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
    return failures == 0 ? 0 : 1;
}
