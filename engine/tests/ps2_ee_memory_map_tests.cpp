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
    std::uint64_t low = 0, high = 0;

    check(memory.write(0x100, 4, 0x12345678, &error), "write little-endian word to RAM");
    check(memory.read(0x100, 4, value, &error) && value == 0x12345678,
          "read word from physical RAM");
    check(memory.read(0x20000100, 4, value, &error) && value == 0x12345678,
          "main RAM mirror resolves to same bytes");
    check(memory.read(0x80000100, 4, value, &error) && value == 0x12345678,
          "KSEG0 cached alias resolves to RAM");
    check(memory.read(0xA0000100, 4, value, &error) && value == 0x12345678,
          "KSEG1 uncached alias resolves to RAM");

    // The accelerated uncached window maps physical RAM starting at offset 1 MiB.
    retroshelf::Ps2EeMemoryMap full_ram;
    check(full_ram.write(0x00100000U, 4, 0xCAFEBABEU, &error),
          "seed RAM at the accelerated alias's 1 MiB offset");
    check(full_ram.read(retroshelf::Ps2EeMemoryMap::MainRamAcceleratedBase,
                        4, value, &error) && value == 0xCAFEBABEU,
          "accelerated uncached alias resolves to physical RAM offset 1 MiB");
    check(full_ram.write(retroshelf::Ps2EeMemoryMap::MainRamAcceleratedBase + 4U,
                         4, 0x1234ABCDU, &error) &&
              full_ram.read(0x00100004U, 4, value, &error) && value == 0x1234ABCDU,
          "writes through accelerated alias update the same physical RAM");
    check(!full_ram.read(retroshelf::Ps2EeMemoryMap::MainRamAcceleratedEnd,
                         4, value, &error) && !error.empty(),
          "address immediately beyond accelerated RAM alias is rejected");

    check(memory.write(0x104, 1, 0xFE, &error), "byte write succeeds");
    check(memory.read(0x104, 1, value, &error) && value == 0xFE,
          "byte read returns unsigned byte value");
    check(!memory.read(0x101, 4, value, &error) && !error.empty(),
          "unaligned word access is rejected");
    check(!memory.write(0x40000000, 4, 1, &error) && !error.empty(),
          "unmapped address is rejected");
    check(!memory.write(0x100000, 8, 1, &error) && !error.empty(),
          "out-of-range access is rejected for small configured RAM");

    check(memory.scratchpad_size() == retroshelf::Ps2EeMemoryMap::ScratchpadSize,
          "EE scratchpad has the expected 16 KiB capacity");
    check(memory.write(retroshelf::Ps2EeMemoryMap::ScratchpadBase, 4, 0x78563412U, &error),
          "write word to EE scratchpad");
    check(memory.read(retroshelf::Ps2EeMemoryMap::ScratchpadBase, 4, value, &error) &&
              value == 0x78563412U,
          "read word from EE scratchpad");
    check(memory.read(0x70000000U, 4, value, &error) && value == 0x78563412U,
          "scratchpad bytes remain accessible at the architectural base");
    check(memory.write128(0x70000010U, 0x0123456789ABCDEFULL,
                          0xFEDCBA9876543210ULL, &error),
          "write aligned 128-bit value to EE scratchpad");
    check(memory.read128(0x70000010U, low, high, &error) &&
              low == 0x0123456789ABCDEFULL && high == 0xFEDCBA9876543210ULL,
          "read aligned 128-bit value from EE scratchpad");
    check(!memory.write(0x70003FFEU, 4, 1, &error),
          "scratchpad access crossing the 16 KiB boundary is rejected");
    check(!memory.write(0x70000001U, 4, 1, &error) && !error.empty(),
          "misaligned scratchpad access is rejected");

    retroshelf::Ps2EeMemoryMap small(256);
    check(!small.read(0x100, 8, value, &error), "access beyond configured RAM is rejected");
    check(!small.read(0xBFC00000U, 4, value, &error) &&
              error.find("No BIOS image is mapped") != std::string::npos,
          "BIOS-window read without a loaded ROM reports a BIOS-specific error");
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
    check(!memory.read(0xBFC00010U, 4, value, &error) &&
              error.find("BIOS read exceeds the loaded ROM image") != std::string::npos,
          "read beyond loaded BIOS image reports a ROM-boundary error");
    check(!memory.read128(0xBFC00000U + 0x10U, low, high, &error) &&
              error.find("BIOS read exceeds the loaded ROM image") != std::string::npos,
          "128-bit read beyond loaded BIOS image reports a ROM-boundary error");
    check(!memory.write(0xBFC00000U, 4, 0xDEADBEEFU, &error) && !error.empty(),
          "writes to BIOS ROM are rejected");
    check(memory.read(0x1FC00000U, 4, value, &error) && value == 0x13121110U,
          "rejected BIOS write leaves ROM contents unchanged");
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
