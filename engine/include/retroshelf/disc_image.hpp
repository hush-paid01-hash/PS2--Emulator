#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf::media {

struct DiscImageInfo {
    bool valid{false};
    std::uint32_t volume_sectors{0};
    std::uint16_t logical_block_size{0};
    std::string volume_id;
    std::string error;
};

// Minimal ISO-9660 probe. It does not yet boot PS2 executables.
DiscImageInfo inspect_iso9660(const std::string& path);

// Reads one 2048-byte logical sector from a validated ISO-9660 image.
bool read_iso_sector(const std::string& path, std::uint32_t lba,
                     std::vector<std::uint8_t>& sector, std::string* error = nullptr);

} // namespace retroshelf::media
