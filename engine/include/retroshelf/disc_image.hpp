#pragma once

#include <cstdint>
#include <string>

namespace retroshelf::media {

// Minimal ISO-9660 primary-volume-descriptor probe. This identifies a common
// ISO layout; it does not yet parse the PS2 filesystem or boot executable.
struct DiscImageInfo {
    bool valid{false};
    std::uint32_t volume_sectors{0};
    std::uint16_t logical_block_size{0};
    std::string volume_id;
    std::string error;
};

DiscImageInfo inspect_iso9660(const std::string& path);

} // namespace retroshelf::media
