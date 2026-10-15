#include "retroshelf/elf_ram_loader.hpp"

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
    std::cout << "ELF RAM loader tests passed\n";
}
