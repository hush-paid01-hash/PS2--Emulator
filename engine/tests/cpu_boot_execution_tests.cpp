#include "retroshelf/cpu.hpp"
#include "retroshelf/elf_ram_loader.hpp"
#include "retroshelf/ps2_ee_memory_map.hpp"

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
    // Execute from KSEG0 and prove LW/SW share memory through the KSEG1 alias.
    retroshelf::Ps2EeMemoryMap ee;
    retroshelf::Cpu mapped_cpu;
    mapped_cpu.set_pc(0x80001000U);
    mapped_cpu.set_reg(16, 0xA0000000U); // s0 points at KSEG1 RAM alias
    assert(ee.write(0x80001000U, 4, (0x09U << 26U) | (8U << 16U) | 7U, &error));
    assert(ee.write(0x80001004U, 4, (0x2BU << 26U) | (16U << 21U) | (8U << 16U) | 0x20U, &error)); // SW t0,0x20(s0)
    assert(ee.write(0x80001008U, 4, (0x23U << 26U) | (16U << 21U) | (9U << 16U) | 0x20U, &error)); // LW t1,0x20(s0)
    assert(mapped_cpu.step_from_ee_memory(ee, &error));
    assert(mapped_cpu.reg(8) == 7);
    assert(mapped_cpu.step_from_ee_memory(ee, &error));
    std::uint64_t stored = 0;
    assert(ee.read(0x00000020U, 4, stored, &error));
    assert(stored == 7);
    assert(mapped_cpu.step_from_ee_memory(ee, &error));
    assert(mapped_cpu.reg(9) == 7);
    std::cout << "CPU boot execution tests passed\n";
}
