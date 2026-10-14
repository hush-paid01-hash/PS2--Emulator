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

struct IsoDirectoryEntry {
    std::string name;
    std::uint32_t lba{0};
    std::uint32_t byte_size{0};
    bool directory{false};
};

struct Ps2BootInfo {
    bool found{false};
    std::string system_config_path;
    std::string boot_path;
    std::uint32_t executable_lba{0};
    std::uint32_t executable_size{0};
    std::string error;
};

DiscImageInfo inspect_iso9660(const std::string& path);
bool read_iso_sector(const std::string& path, std::uint32_t lba,
                     std::vector<std::uint8_t>& sector, std::string* error = nullptr);
bool read_iso_file(const std::string& path, std::uint32_t lba, std::uint32_t byte_size,
                   std::vector<std::uint8_t>& contents, std::string* error = nullptr);
bool read_iso_directory(const std::string& path, std::vector<IsoDirectoryEntry>& entries,
                        std::string* error = nullptr);
Ps2BootInfo find_ps2_boot_executable(const std::string& path);

} // namespace retroshelf::media
