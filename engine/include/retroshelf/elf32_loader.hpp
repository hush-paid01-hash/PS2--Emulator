#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf {

// Loads little-endian ELF32 MIPS PT_LOAD segments into the supplied EE RAM.
// This is an executable-image loader, not a BIOS or full PS2 boot implementation.
struct ElfLoadResult {
    bool ok{false};
    std::uint32_t entry_point{0};
    std::size_t segments_loaded{0};
    std::string error;
};

class Elf32Loader {
public:
    static ElfLoadResult load_mips_executable(
        const std::vector<std::uint8_t>& image,
        std::vector<std::uint8_t>& ee_ram);
};

} // namespace retroshelf
