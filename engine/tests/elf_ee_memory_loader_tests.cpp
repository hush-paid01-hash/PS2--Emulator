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
    std::cout << "ELF EE memory-map loader tests passed\n";
}
