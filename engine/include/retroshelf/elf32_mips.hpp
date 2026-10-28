#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf {

struct Elf32LoadSegment {
    std::uint32_t virtual_address{};
    std::uint32_t memory_size{};
    std::uint32_t flags{};
    std::vector<std::uint8_t> file_bytes;
};

struct Elf32MipsImage {
    std::uint32_t entry_point{};
    std::vector<Elf32LoadSegment> load_segments;
};

// Validates a little-endian ELF32 MIPS executable and extracts PT_LOAD segments.
// It does not map segments into PS2 RAM or execute instructions.
bool parse_elf32_mips(const std::vector<std::uint8_t>& bytes,
                      Elf32MipsImage& image, std::string* error = nullptr);

} // namespace retroshelf
