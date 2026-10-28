#pragma once

#include "retroshelf/elf32_mips.hpp"
#include "retroshelf/ps2_ee_memory_map.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf {

bool load_elf32_into_ram(const Elf32MipsImage& image,
                         std::uint32_t ram_base,
                         std::vector<std::uint8_t>& ram,
                         std::uint32_t& entry_point,
                         std::string* error = nullptr);

// Loads PT_LOAD segments into mapped EE main RAM, supporting the RAM aliases
// currently implemented by Ps2EeMemoryMap. RAM is unchanged on validation failure.
bool load_elf32_into_ee_memory(const Elf32MipsImage& image,
                               Ps2EeMemoryMap& memory,
                               std::uint32_t& entry_point,
                               std::string* error = nullptr);

} // namespace retroshelf
