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


    // Reject an entry point outside all executable PT_LOAD segments without
    // touching RAM, even when the segment itself fits in memory.
    retroshelf::Elf32MipsImage invalid_entry_image;
    invalid_entry_image.entry_point = 0x80002000U;
    retroshelf::Elf32LoadSegment data_segment;
    data_segment.virtual_address = 0x80001000U;
    data_segment.memory_size = 4;
    data_segment.flags = 4; // readable, not executable
    data_segment.file_bytes = {9, 8, 7, 6};
    invalid_entry_image.load_segments.push_back(data_segment);
    const auto before_invalid_entry = ee_memory.main_ram();
    entry = 123;
    assert(!retroshelf::load_elf32_into_ee_memory(
        invalid_entry_image, ee_memory, entry, &error));
    assert(entry == 0);
    assert(ee_memory.main_ram() == before_invalid_entry);
    assert(!error.empty());


    // Malformed PT_LOAD metadata must fail before modifying EE RAM.
    retroshelf::Elf32MipsImage malformed_image;
    malformed_image.entry_point = 0x80003000U;
    retroshelf::Elf32LoadSegment malformed_segment;
    malformed_segment.virtual_address = 0x80003000U;
    malformed_segment.memory_size = 2;
    malformed_segment.flags = 5;
    malformed_segment.file_bytes = {0x10, 0x20, 0x30};
    malformed_image.load_segments.push_back(malformed_segment);
    const auto before_malformed = ee_memory.main_ram();
    entry = 456;
    assert(!retroshelf::load_elf32_into_ee_memory(
        malformed_image, ee_memory, entry, &error));
    assert(entry == 0);
    assert(ee_memory.main_ram() == before_malformed);
    assert(!error.empty());

    // Instruction fetches require word-aligned MIPS entry points.
    retroshelf::Elf32MipsImage unaligned_entry_image;
    unaligned_entry_image.entry_point = 0x80004002U;
    retroshelf::Elf32LoadSegment unaligned_segment;
    unaligned_segment.virtual_address = 0x80004000U;
    unaligned_segment.memory_size = 8;
    unaligned_segment.flags = 5;
    unaligned_segment.file_bytes = {1, 2, 3, 4, 5, 6, 7, 8};
    unaligned_entry_image.load_segments.push_back(unaligned_segment);
    const auto before_unaligned_entry = ee_memory.main_ram();
    entry = 789;
    assert(!retroshelf::load_elf32_into_ee_memory(
        unaligned_entry_image, ee_memory, entry, &error));
    assert(entry == 0);
    assert(ee_memory.main_ram() == before_unaligned_entry);
    assert(!error.empty());

    std::cout << "ELF RAM loader tests passed\n";
}
