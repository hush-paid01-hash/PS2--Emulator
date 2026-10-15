#include "retroshelf/elf_ram_loader.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
retroshelf::Elf32MipsImage valid_image() {
    retroshelf::Elf32MipsImage image;
    image.entry_point = 0x1004;
    retroshelf::Elf32LoadSegment segment;
    segment.virtual_address = 0x1000;
    segment.memory_size = 8;
    segment.flags = 5; // executable and readable
    segment.file_bytes = {0x11, 0x22, 0x33, 0x44};
    image.load_segments.push_back(segment);
    return image;
}
}

int main() {
    std::vector<std::uint8_t> ram(0x2000, 0xAA);
    retroshelf::SystemBus bus(ram);
    std::uint32_t entry = 0xFFFFFFFFU;
    std::string error;
    const auto image = valid_image();
    assert(retroshelf::load_elf32_into_ram(image, 0, bus, entry, &error));
    assert(entry == 0x1004);
    assert(ram[0x1000] == 0x11 && ram[0x1001] == 0x22);
    assert(ram[0x1002] == 0x33 && ram[0x1003] == 0x44);
    for (std::size_t i = 0x1004; i < 0x1008; ++i) assert(ram[i] == 0);
    assert(ram[0x0FFF] == 0xAA && ram[0x1008] == 0xAA);
    assert(error.empty());

    auto invalid = valid_image();
    invalid.load_segments[0].virtual_address = 0x3000;
    const auto before = ram;
    entry = 0xFFFFFFFFU;
    assert(!retroshelf::load_elf32_into_ram(invalid, 0, bus, entry, &error));
    assert(entry == 0);
    assert(ram == before); // validate all segments before touching RAM

    invalid = valid_image();
    invalid.entry_point = 0x1010;
    assert(!retroshelf::load_elf32_into_ram(invalid, 0, bus, entry, &error));
    assert(ram == before);
    std::cout << "ELF RAM loader tests passed\n";
}
