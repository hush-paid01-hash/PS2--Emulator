#include "retroshelf/elf_ram_loader.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

int main() {
    retroshelf::Elf32MipsImage image;
    image.entry_point = 0x00100000;
    retroshelf::Elf32LoadSegment segment;
    segment.virtual_address = 0x00100000;
    segment.memory_size = 8;
    segment.flags = 5; // readable + executable
    segment.file_bytes = {0x11, 0x22, 0x33, 0x44};
    image.load_segments.push_back(segment);

    constexpr std::uint32_t base = 0;
    std::vector<std::uint8_t> ram(2 * 1024 * 1024, 0xAA);
    std::uint32_t entry = 0;
    std::string error;
    assert(retroshelf::load_elf32_into_ram(image, base, ram, entry, &error));
    assert(entry == image.entry_point);
    assert(ram[0x00100000] == 0x11);
    assert(ram[0x00100003] == 0x44);
    assert(ram[0x00100004] == 0);
    assert(ram[0x00100007] == 0);
    assert(ram[0x00001000] == 0);

    auto preserved = ram;
    image.load_segments[0].virtual_address = 0x00300000;
    entry = 123;
    assert(!retroshelf::load_elf32_into_ram(image, base, ram, entry, &error));
    assert(entry == 0);
    assert(ram == preserved);
    assert(!error.empty());

    std::vector<std::uint8_t> empty;
    assert(!retroshelf::load_elf32_into_ram(image, base, empty, entry, &error));
    assert(entry == 0);

    // EE loading accepts KSEG0 aliases and writes through to physical RAM.
    retroshelf::Ps2EeMemoryMap ee_memory(2 * 1024 * 1024);
    std::fill(ee_memory.main_ram().begin(), ee_memory.main_ram().end(), 0x5A);
    retroshelf::Elf32MipsImage ee_image;
    ee_image.entry_point = 0x80001000U;
    retroshelf::Elf32LoadSegment ee_segment;
    ee_segment.virtual_address = 0x80001000U;
    ee_segment.memory_size = 8;
    ee_segment.flags = 5;
    ee_segment.file_bytes = {0xAA, 0xBB, 0xCC, 0xDD};
    ee_image.load_segments.push_back(ee_segment);
    entry = 0;
    assert(retroshelf::load_elf32_into_ee_memory(ee_image, ee_memory, entry, &error));
    assert(entry == 0x80001000U);
    assert(ee_memory.main_ram()[0x1000] == 0xAA);
    assert(ee_memory.main_ram()[0x1003] == 0xDD);
    assert(ee_memory.main_ram()[0x1004] == 0);
    assert(ee_memory.main_ram()[0x1007] == 0);

    // Physical and KSEG0 aliases of the same RAM range must not overlap.
    retroshelf::Elf32LoadSegment alias_segment;
    alias_segment.virtual_address = 0x00001004U;
    alias_segment.memory_size = 4;
    alias_segment.flags = 4;
    alias_segment.file_bytes = {1, 2, 3, 4};
    ee_image.load_segments.push_back(alias_segment);
    const auto ee_preserved = ee_memory.main_ram();
    entry = 99;
    assert(!retroshelf::load_elf32_into_ee_memory(ee_image, ee_memory, entry, &error));
    assert(entry == 0);
    assert(ee_memory.main_ram() == ee_preserved);
    assert(!error.empty());

    std::cout << "ELF RAM loader tests passed\n";
}
