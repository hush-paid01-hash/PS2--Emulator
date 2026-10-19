#include "retroshelf/elf_ram_loader.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

int main() {
    retroshelf::Ps2EeMemoryMap memory(2U * 1024U * 1024U);
    retroshelf::Elf32MipsImage image;
    image.entry_point = 0x80100000U;
    retroshelf::Elf32LoadSegment segment;
    segment.virtual_address = 0x80100000U;
    segment.memory_size = 8;
    segment.flags = 5;
    segment.file_bytes = {0x07, 0x00, 0x08, 0x24, 0x05, 0x00, 0x09, 0x24};
    image.load_segments.push_back(segment);

    std::uint32_t entry = 0;
    std::string error;
    assert(retroshelf::load_elf32_into_ee_memory(image, memory, entry, &error));
    assert(entry == 0x80100000U);
    std::uint64_t value = 0;
    assert(memory.read(0x00100000U, 4, value, &error) && value == 0x24080007U);
    assert(memory.read(0xA0100000U, 4, value, &error) && value == 0x24080007U);

    assert(memory.write(0x00100008U, 4, 0xDEADBEEFU, &error));
    image.load_segments[0].memory_size = 12;
    assert(retroshelf::load_elf32_into_ee_memory(image, memory, entry, &error));
    assert(memory.read(0x00100008U, 4, value, &error) && value == 0);

    const auto before = memory.main_ram();
    image.load_segments[0].virtual_address = 0x80300000U;
    entry = 123;
    assert(!retroshelf::load_elf32_into_ee_memory(image, memory, entry, &error));
    assert(entry == 0);
    assert(memory.main_ram() == before);
    assert(!error.empty());

    image.load_segments[0].virtual_address = 0x80100000U;
    image.load_segments[0].memory_size = 8;
    image.load_segments[0].flags = 4;
    assert(!retroshelf::load_elf32_into_ee_memory(image, memory, entry, &error));
    assert(memory.main_ram() == before);
    // The EE loader must treat the end of an executable PT_LOAD range as
    // exclusive and leave physical RAM untouched on rejection.
    retroshelf::Elf32MipsImage end_entry_image;
    end_entry_image.entry_point = 0x80100008U;
    retroshelf::Elf32LoadSegment end_entry_segment;
    end_entry_segment.virtual_address = 0x80100000U;
    end_entry_segment.memory_size = 8;
    end_entry_segment.flags = 5;
    end_entry_segment.file_bytes = {1, 2, 3, 4, 5, 6, 7, 8};
    end_entry_image.load_segments.push_back(end_entry_segment);
    const auto before_end_entry = memory.main_ram();
    entry = 789;
    assert(!retroshelf::load_elf32_into_ee_memory(
        end_entry_image, memory, entry, &error));
    assert(entry == 0);
    assert(memory.main_ram() == before_end_entry);
    assert(!error.empty());

    std::cout << "ELF EE memory-map loader tests passed\n";
}
