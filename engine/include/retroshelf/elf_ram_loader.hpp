#pragma once

#include "retroshelf/elf32_mips.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf {

// Copies an ELF load plan into caller-owned flat RAM using virtual addresses
// as offsets from ram_base. This is a bounded staging helper, not the PS2
// virtual-to-physical mapper and not a CPU execution/BIOS implementation.
bool load_elf32_into_ram(const Elf32MipsImage& image,
                         std::uint32_t ram_base,
                         std::vector<std::uint8_t>& ram,
                         std::uint32_t& entry_point,
                         std::string* error = nullptr);

} // namespace retroshelf
