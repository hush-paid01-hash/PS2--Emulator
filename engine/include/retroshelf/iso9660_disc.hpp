#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace retroshelf {

// Read-only ISO-9660/Joliet-independent reader for 2048-byte ISO images and
// raw 2352-byte BIN/IMG sectors. This is disc-image infrastructure, not a
// complete PS2 CD/DVD drive or CHD decoder.
class Iso9660Disc {
public:
    bool open(const std::filesystem::path& path, std::string* error = nullptr);
    bool is_open() const;
    std::uint64_t sector_count() const;
    bool read_sector(std::uint32_t lba, std::vector<std::uint8_t>& out,
                     std::string* error = nullptr) const;
    bool read_file(std::string iso_path, std::vector<std::uint8_t>& out,
                   std::string* error = nullptr) const;

private:
    struct DirectoryRecord {
        std::uint32_t extent{0};
        std::uint32_t size{0};
        std::uint8_t flags{0};
        std::string name;
    };
    bool read_record(std::uint32_t lba, std::uint32_t offset,
                     DirectoryRecord& record, std::uint32_t& record_length,
                     std::string* error) const;
    bool find_record(const std::string& iso_path, DirectoryRecord& record,
                     std::string* error) const;

    std::filesystem::path path_;
    std::uint64_t file_size_{0};
    std::uint32_t physical_sector_size_{0};
    std::uint32_t data_offset_{0};
    std::uint64_t sectors_{0};
    DirectoryRecord root_{};
    bool open_{false};
};

} // namespace retroshelf
