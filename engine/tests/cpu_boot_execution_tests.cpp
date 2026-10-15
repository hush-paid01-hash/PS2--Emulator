#include "retroshelf/cpu.hpp"
#include "retroshelf/elf_ram_loader.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
void append_instruction(std::vector<std::uint8_t>& bytes, std::uint32_t instruction) {
    bytes.push_back(static_cast<std::uint8_t>(instruction));
    bytes.push_back(static_cast<std::uint8_t>(instruction >> 8U));
    bytes.push_back(static_cast<std::uint8_t>(instruction >> 16U));
    bytes.push_back(static_cast<std::uint8_t>(instruction >> 24U));
}
}

int main() {
    retroshelf::Elf32MipsImage image;
    image.entry_point = 0x1000;
    retroshelf::Elf32LoadSegment segment;
    segment.virtual_address = 0x1000;
    segment.memory_size = 12;
    segment.flags = 5; // readable + executable
    append_instruction(segment.file_bytes, (0x09U << 26U) | (8U << 16U) | 7U);
    append_instruction(segment.file_bytes, (0x09U << 26U) | (9U << 16U) | 5U);
    image.load_segments.push_back(segment);

    std::vector<std::uint8_t> ram(0x2000, 0xA5);
    std::uint32_t entry = 0;
    std::string error;
    assert(retroshelf::load_elf32_into_ram(image, 0, ram, entry, &error));
    assert(entry == 0x1000);
    assert(ram[0x1000] == 0x07);
    assert(ram[0x1004] == 0x05);
    assert(ram[0x1008] == 0);

    retroshelf::Cpu cpu;
    cpu.set_pc(entry);
    assert(cpu.step_from_ram(ram, 0, &error));
    assert(cpu.reg(8) == 7);
    assert(cpu.pc() == 0x1004);
    assert(cpu.step_from_ram(ram, 0, &error));
    assert(cpu.reg(9) == 5);
    assert(cpu.pc() == 0x1008);
    std::cout << "CPU boot execution tests passed\n";
}
