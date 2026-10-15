#include "retroshelf/iso9660_disc.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>

namespace retroshelf {
namespace {
std::uint16_t le16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0] | (static_cast<std::uint16_t>(p[1]) << 8U));
}
std::uint32_t le32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) |
        (static_cast<std::uint32_t>(p[1]) << 8U) |
        (static_cast<std::uint32_t>(p[2]) << 16U) |
        (static_cast<std::uint32_t>(p[3]) << 24U);
}
std::string upper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return value;
}
std::string normalize_name(std::string value) {
    const auto version = value.find(';');
    if (version != std::string::npos) value.resize(version);
    if (value.size() > 1 && value.back() == '.') value.pop_back();
    return upper(value);
}
}

bool Iso9660Disc::open(const std::filesystem::path& path, std::string* error) {
    open_ = false;
    path_ = path;
    physical_sector_size_ = data_offset_ = 0;
    sectors_ = file_size_ = 0;
    root_ = {};
    std::error_code ec;
    file_size_ = std::filesystem::file_size(path, ec);
    if (ec || file_size_ < 17U * 2048U) {
        if (error) *error = "Image is missing, unreadable, or too small for an ISO-9660 volume descriptor";
        return false;
    }

    // Detect standard 2048-byte sectors and raw CD sectors with a 24-byte
    // sync/header prefix. We validate the PVD identifier rather than guessing
    // from the filename extension.
    std::ifstream input(path, std::ios::binary);
    if (!input) { if (error) *error = "Unable to open disc image"; return false; }
    std::vector<std::uint8_t> probe(2048);
    const auto has_pvd = [&](std::uint32_t physical, std::uint32_t offset) {
        const auto pos = static_cast<std::streamoff>(16ULL * physical + offset);
        input.clear(); input.seekg(pos);
        if (!input.read(reinterpret_cast<char*>(probe.data()), static_cast<std::streamsize>(probe.size()))) return false;
        return probe[0] == 1 && probe[1] == 'C' && probe[2] == 'D' &&
               probe[3] == '0' && probe[4] == '0' && probe[5] == '1' && probe[6] == 1;
    };
    if (has_pvd(2048, 0)) { physical_sector_size_ = 2048; data_offset_ = 0; }
    else if (has_pvd(2352, 24)) { physical_sector_size_ = 2352; data_offset_ = 24; }
    else { if (error) *error = "No supported ISO-9660 primary volume descriptor found (2048-byte ISO or raw 2352-byte BIN/IMG required)"; return false; }

    sectors_ = file_size_ / physical_sector_size_;
    const std::uint32_t root_offset = 156;
    const std::uint32_t record_length = probe[root_offset];
    if (record_length < 34 || root_offset + record_length > probe.size()) {
        if (error) {
            *error = "Malformed ISO-9660 root directory record";
        }
        return false;
    }
    root_.extent = le32(&probe[root_offset + 2]);
    root_.size = le32(&probe[root_offset + 10]);
    root_.flags = probe[root_offset + 25];
    root_.name = "/";
    if ((root_.flags & 2U) == 0 || root_.extent >= sectors_ || root_.size > file_size_) {
        if (error) {
            *error = "Invalid ISO-9660 root directory extent";
        }
        return false;
    }
    open_ = true;
    if (error) error->clear();
    return true;
}

bool Iso9660Disc::is_open() const { return open_; }
std::uint64_t Iso9660Disc::sector_count() const { return sectors_; }

bool Iso9660Disc::read_sector(std::uint32_t lba, std::vector<std::uint8_t>& out,
                              std::string* error) const {
    if (!open_ || lba >= sectors_) {
        if (error) *error = "Disc is not open or sector address is out of range";
        out.clear(); return false;
    }
    std::ifstream input(path_, std::ios::binary);
    if (!input) { if (error) *error = "Unable to reopen disc image"; out.clear(); return false; }
    const auto pos = static_cast<std::uint64_t>(lba) * physical_sector_size_ + data_offset_;
    input.seekg(static_cast<std::streamoff>(pos));
    out.resize(2048);
    if (!input.read(reinterpret_cast<char*>(out.data()), 2048)) {
        if (error) {
            *error = "Short read while reading disc sector";
        }
        out.clear();
        return false;
    }
    if (error) error->clear();
    return true;
}

bool Iso9660Disc::read_record(std::uint32_t lba, std::uint32_t offset,
                              DirectoryRecord& record, std::uint32_t& record_length,
                              std::string* error) const {
    std::vector<std::uint8_t> sector;
    if (!read_sector(lba, sector, error)) return false;
    if (offset >= sector.size() || sector[offset] == 0) {
        record_length = 0; return true;
    }
    record_length = sector[offset];
    if (record_length < 34 || offset + record_length > sector.size()) {
        if (error) {
            *error = "Malformed ISO-9660 directory record";
        }
        return false;
    }
    const auto* p = sector.data() + offset;
    const std::uint8_t name_length = p[32];
    if (33U + name_length > record_length) {
        if (error) {
            *error = "Invalid ISO-9660 filename length";
        }
        return false;
    }
    record.extent = le32(p + 2);
    record.size = le32(p + 10);
    record.flags = p[25];
    if (name_length == 1 && (p[33] == 0 || p[33] == 1)) {
        record.name = p[33] == 0 ? "." : "..";
    } else {
        record.name.assign(reinterpret_cast<const char*>(p + 33), name_length);
    }
    if (record.extent >= sectors_ || record.size > file_size_) {
        if (error) {
            *error = "Directory entry points outside the disc image";
        }
        return false;
    }
    return true;
}

bool Iso9660Disc::find_record(const std::string& iso_path, DirectoryRecord& found,
                              std::string* error) const {
    if (!open_) { if (error) *error = "Disc image is not open"; return false; }
    std::vector<std::string> components;
    std::string part;
    for (char c : iso_path) {
        if (c == '/' || c == '\\') { if (!part.empty()) { components.push_back(normalize_name(part)); part.clear(); } }
        else part.push_back(c);
    }
    if (!part.empty()) components.push_back(normalize_name(part));
    DirectoryRecord current = root_;
    if (components.empty()) { found = root_; return true; }

    for (const auto& component : components) {
        if ((current.flags & 2U) == 0) { if (error) *error = "A path component is not a directory"; return false; }
        bool matched = false;
        std::uint32_t consumed = 0;
        while (consumed < current.size) {
            const std::uint32_t lba = current.extent + consumed / 2048U;
            const std::uint32_t offset = consumed % 2048U;
            std::vector<std::uint8_t> sector;
            if (!read_sector(lba, sector, error)) return false;
            if (sector[offset] == 0) {
                consumed += 2048U - offset;
                continue;
            }
            DirectoryRecord entry;
            std::uint32_t length = 0;
            if (!read_record(lba, offset, entry, length, error)) return false;
            if (length == 0 || length > current.size - consumed) break;
            consumed += length;
            if (normalize_name(entry.name) == component) { current = entry; matched = true; break; }
        }
        if (!matched) { if (error) *error = "ISO-9660 path not found: " + component; return false; }
    }
    found = current;
    if (error) error->clear();
    return true;
}

bool Iso9660Disc::read_file(std::string iso_path, std::vector<std::uint8_t>& out,
                            std::string* error) const {
    DirectoryRecord record;
    if (!find_record(iso_path, record, error)) { out.clear(); return false; }
    if ((record.flags & 2U) != 0) { if (error) *error = "Requested ISO path is a directory"; out.clear(); return false; }
    if (record.size > std::numeric_limits<std::size_t>::max()) {
        if (error) {
            *error = "File is too large for this platform";
        }
        out.clear();
        return false;
    }
    out.clear();
    out.reserve(record.size);
    std::uint32_t remaining = record.size;
    std::uint32_t lba = record.extent;
    while (remaining > 0) {
        std::vector<std::uint8_t> sector;
        if (!read_sector(lba++, sector, error)) { out.clear(); return false; }
        const std::size_t count = std::min<std::size_t>(remaining, sector.size());
        out.insert(out.end(), sector.begin(), sector.begin() + static_cast<std::ptrdiff_t>(count));
        remaining -= static_cast<std::uint32_t>(count);
    }
    if (error) error->clear();
    return true;
}
} // namespace retroshelf
