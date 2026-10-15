#pragma once

#include "retroshelf/elf32_mips.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf {

// Stages a parsed ELF into an explicitly supplied address window.
// This is a bounded loader primitive, not the PS2 virtual-to-physical mapper.
bool stage_elf32_segments(const Elf32MipsImage& image,
                          std::uint32_t window_base,
                          std::size_t window_size,
                          std::vector<std::uint8_t>& memory,
                          std::size_t& entry_offset,
                          std::string* error = nullptr);

} // namespace retroshelf
