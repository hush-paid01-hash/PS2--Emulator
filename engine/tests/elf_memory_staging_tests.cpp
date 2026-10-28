#include "retroshelf/elf_memory_staging.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

int main() {
    retroshelf::Elf32MipsImage image;
    image.entry_point = 0x1004;
    retroshelf::Elf32LoadSegment segment;
    segment.virtual_address = 0x1000;
    segment.memory_size = 8;
    segment.flags = 5; // readable + executable
    segment.file_bytes = {0x11, 0x22, 0x33, 0x44};
    image.load_segments.push_back(segment);

    std::vector<std::uint8_t> memory;
    std::size_t entry = 999;
    std::string error;
    assert(retroshelf::stage_elf32_segments(image, 0x1000, 0x100, memory, entry, &error));
    assert(memory.size() == 0x100);
    assert(memory[0] == 0x11 && memory[3] == 0x44);
    for (std::size_t i = 4; i < 8; ++i) assert(memory[i] == 0);
    assert(entry == 4);
    assert(error.empty());

    assert(!retroshelf::stage_elf32_segments(image, 0x2000, 0x100, memory, entry, &error));
    assert(memory.empty());
    assert(!error.empty());

    image.entry_point = 0x1010;
    assert(!retroshelf::stage_elf32_segments(image, 0x1000, 0x100, memory, entry, &error));
    assert(memory.empty());

    // Overlapping PT_LOAD ranges must be rejected without publishing a
    // partially staged memory image or a stale entry offset.
    retroshelf::Elf32MipsImage overlapping;
    overlapping.entry_point = 0x1000;
    retroshelf::Elf32LoadSegment first;
    first.virtual_address = 0x1000;
    first.memory_size = 8;
    first.flags = 5;
    first.file_bytes = {1, 2, 3, 4};
    retroshelf::Elf32LoadSegment second;
    second.virtual_address = 0x1004;
    second.memory_size = 8;
    second.flags = 4;
    second.file_bytes = {5, 6, 7, 8};
    overlapping.load_segments.push_back(first);
    overlapping.load_segments.push_back(second);
    entry = 777;
    assert(!retroshelf::stage_elf32_segments(
        overlapping, 0x1000, 0x100, memory, entry, &error));
    assert(memory.empty());
    assert(entry == 0);
    assert(!error.empty());

    // A zero-sized executable segment cannot contain an entry point.
    retroshelf::Elf32MipsImage empty_segment_image;
    empty_segment_image.entry_point = 0x1000;
    retroshelf::Elf32LoadSegment empty_segment;
    empty_segment.virtual_address = 0x1000;
    empty_segment.memory_size = 0;
    empty_segment.flags = 5;
    empty_segment_image.load_segments.push_back(empty_segment);
    entry = 777;
    assert(!retroshelf::stage_elf32_segments(
        empty_segment_image, 0x1000, 0x100, memory, entry, &error));
    assert(memory.empty());
    assert(entry == 0);
    assert(!error.empty());

    std::cout << "ELF memory staging tests passed\n";
}
