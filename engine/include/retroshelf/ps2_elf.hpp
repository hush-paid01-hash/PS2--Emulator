#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf::media {

struct ElfLoadSegment {
    std::uint32_t virtual_address{0};
    std::uint32_t memory_size{0};
    std::uint32_t file_size{0};
    std::uint32_t flags{0};
    std::vector<std::uint8_t> bytes; // file bytes followed by zero-filled BSS
};

struct Ps2ElfImage {
    bool valid{false};
    std::uint32_t entry_point{0};
    std::uint32_t global_pointer{0};
    std::vector<ElfLoadSegment> segments;
    std::string error;
};

// Parses a 32-bit, little-endian MIPS ELF executable and extracts PT_LOAD
// segments. This is a loader foundation, not a complete PS2 boot implementation.
Ps2ElfImage parse_ps2_elf(const std::vector<std::uint8_t>& image);

// Finds SYSTEM.CNF's BOOT/BOOT2 file on an ISO and parses that file as ELF.
Ps2ElfImage load_ps2_boot_elf_from_disc(const std::string& iso_path);

} // namespace retroshelf::media
