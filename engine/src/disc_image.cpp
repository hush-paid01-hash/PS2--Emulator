#include "retroshelf/disc_image.hpp"

#include <array>
#include <cctype>
#include <fstream>
#include <limits>
#include <utility>

namespace retroshelf::media {
namespace {
constexpr std::uint64_t SectorSize = 2048;
constexpr std::uint64_t PvdOffset = 16 * SectorSize;
constexpr std::size_t PvdSize = 2048;
std::uint16_t read_le16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0] | (static_cast<std::uint16_t>(p[1]) << 8U));
}
std::uint32_t read_le32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8U)
        | (static_cast<std::uint32_t>(p[2]) << 16U) | (static_cast<std::uint32_t>(p[3]) << 24U);
}
std::uint32_t read_be32(const std::uint8_t* p) {
    return (static_cast<std::uint32_t>(p[0]) << 24U) | (static_cast<std::uint32_t>(p[1]) << 16U)
        | (static_cast<std::uint32_t>(p[2]) << 8U) | static_cast<std::uint32_t>(p[3]);
}
bool fail(std::string* error, const std::string& message) {
    if (error) *error = message;
    return false;
}
}

DiscImageInfo inspect_iso9660(const std::string& path) {
    DiscImageInfo result;
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) { result.error = "Could not open image file"; return result; }
    const auto end = file.tellg();
    if (end < 0 || static_cast<std::uint64_t>(end) < PvdOffset + PvdSize) {
        result.error = "File is too small to contain an ISO-9660 volume descriptor"; return result;
    }
    file.seekg(static_cast<std::streamoff>(PvdOffset), std::ios::beg);
    std::array<std::uint8_t, PvdSize> pvd{};
    if (!file.read(reinterpret_cast<char*>(pvd.data()), static_cast<std::streamsize>(pvd.size()))) {
        result.error = "Could not read ISO-9660 volume descriptor"; return result;
    }
    if (pvd[0] != 1 || pvd[1] != 'C' || pvd[2] != 'D' || pvd[3] != '0'
        || pvd[4] != '0' || pvd[5] != '1' || pvd[6] != 1) {
        result.error = "ISO-9660 primary volume descriptor signature not found at sector 16"; return result;
    }
    const auto sectors_le = read_le32(pvd.data() + 80);
    const auto sectors_be = read_be32(pvd.data() + 84);
    const auto block_le = read_le16(pvd.data() + 128);
    const auto block_be = static_cast<std::uint16_t>((static_cast<std::uint16_t>(pvd[130]) << 8U) | pvd[131]);
    if (sectors_le == 0 || sectors_le != sectors_be || block_le != SectorSize || block_le != block_be) {
        result.error = "ISO-9660 volume size or logical block size is inconsistent"; return result;
    }
    const auto declared_bytes = static_cast<std::uint64_t>(sectors_le) * block_le;
    if (declared_bytes > static_cast<std::uint64_t>(end)) {
        result.error = "Image is shorter than its declared ISO-9660 volume size"; return result;
    }
    result.valid = true; result.volume_sectors = sectors_le; result.logical_block_size = block_le;
    for (std::size_t i = 40; i < 72 && pvd[i] != 0; ++i) {
        const auto c = static_cast<unsigned char>(pvd[i]);
        result.volume_id.push_back(static_cast<char>(std::isprint(c) ? c : ' '));
    }
    while (!result.volume_id.empty() && result.volume_id.back() == ' ') result.volume_id.pop_back();
    return result;
}

bool read_iso_sector(const std::string& path, std::uint32_t lba,
                     std::vector<std::uint8_t>& sector, std::string* error) {
    const auto info = inspect_iso9660(path);
    if (!info.valid) return fail(error, info.error);
    if (lba >= info.volume_sectors) return fail(error, "Requested logical block is outside the ISO volume");
    const auto offset = static_cast<std::uint64_t>(lba) * SectorSize;
    if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()))
        return fail(error, "Requested logical block offset is not representable");
    std::ifstream file(path, std::ios::binary);
    if (!file) return fail(error, "Could not open image file");
    file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    std::vector<std::uint8_t> candidate(static_cast<std::size_t>(SectorSize));
    if (!file.read(reinterpret_cast<char*>(candidate.data()), static_cast<std::streamsize>(candidate.size())))
        return fail(error, "Could not read requested ISO logical block");
    sector = std::move(candidate);
    if (error) error->clear();
    return true;
}

bool read_iso_file(const std::string& path, std::uint32_t lba, std::uint32_t byte_size,
                   std::vector<std::uint8_t>& contents, std::string* error) {
    const auto info = inspect_iso9660(path);
    if (!info.valid) return fail(error, info.error);
    const std::uint64_t start = static_cast<std::uint64_t>(lba) * SectorSize;
    const std::uint64_t end = start + byte_size;
    const std::uint64_t volume_end = static_cast<std::uint64_t>(info.volume_sectors) * SectorSize;
    if (start > volume_end || end < start || end > volume_end)
        return fail(error, "File extent lies outside the ISO volume");
    if (byte_size > static_cast<std::uint32_t>(std::numeric_limits<std::size_t>::max()))
        return fail(error, "Requested file is too large for this host");
    std::ifstream file(path, std::ios::binary);
    if (!file) return fail(error, "Could not open image file");
    file.seekg(static_cast<std::streamoff>(start), std::ios::beg);
    std::vector<std::uint8_t> candidate(byte_size);
    if (byte_size && !file.read(reinterpret_cast<char*>(candidate.data()), static_cast<std::streamsize>(byte_size)))
        return fail(error, "Could not read ISO file extent");
    contents = std::move(candidate);
    if (error) error->clear();
    return true;
}

bool read_iso_directory(const std::string& path, std::vector<IsoDirectoryEntry>& entries, std::string* error) {
    const auto info = inspect_iso9660(path);
    if (!info.valid) return fail(error, info.error);
    std::vector<std::uint8_t> pvd;
    if (!read_iso_sector(path, 16, pvd, error)) return false;
    const auto root_record_length = pvd[156];
    if (root_record_length < 34 || 156U + root_record_length > pvd.size())
        return fail(error, "ISO root directory record is malformed");
    const auto* root = pvd.data() + 156;
    const auto root_lba = read_le32(root + 2);
    const auto root_size = read_le32(root + 10);
    std::vector<std::uint8_t> bytes;
    if (!read_iso_file(path, root_lba, root_size, bytes, error)) return false;
    std::vector<IsoDirectoryEntry> parsed;
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const auto record_length = bytes[offset];
        if (record_length == 0) {
            offset = ((offset / SectorSize) + 1) * SectorSize;
            continue;
        }
        if (record_length < 34 || offset + record_length > bytes.size())
            return fail(error, "ISO directory contains a malformed record");
        const auto* record = bytes.data() + offset;
        const auto name_length = record[32];
        if (33U + name_length > record_length) return fail(error, "ISO directory name exceeds record");
        if (!(name_length == 1 && (record[33] == 0 || record[33] == 1))) {
            IsoDirectoryEntry entry;
            entry.lba = read_le32(record + 2);
            entry.byte_size = read_le32(record + 10);
            entry.directory = (record[25] & 0x02U) != 0;
            entry.name.assign(reinterpret_cast<const char*>(record + 33), name_length);
            const auto version = entry.name.find(';');
            if (version != std::string::npos) entry.name.resize(version);
            parsed.push_back(std::move(entry));
        }
        offset += record_length;
    }
    entries = std::move(parsed);
    if (error) error->clear();
    return true;
}

Ps2BootInfo find_ps2_boot_executable(const std::string& path) {
    Ps2BootInfo result;
    std::vector<IsoDirectoryEntry> root;
    if (!read_iso_directory(path, root, &result.error)) return result;
    for (const auto& entry : root) {
        if (!entry.directory && entry.name == "SYSTEM.CNF") {
            result.system_config_path = entry.name;
            std::vector<std::uint8_t> bytes;
            if (!read_iso_file(path, entry.lba, entry.byte_size, bytes, &result.error)) return result;
            const std::string config(bytes.begin(), bytes.end());
            const std::string key = "BOOT2";
            auto pos = config.find(key);
            if (pos == std::string::npos) pos = config.find("BOOT");
            if (pos == std::string::npos) {
                result.error = "SYSTEM.CNF has no BOOT/BOOT2 entry"; return result;
            }
            const auto equal = config.find('=', pos);
            if (equal == std::string::npos) { result.error = "Malformed boot path in SYSTEM.CNF"; return result; }
            auto begin = config.find_first_not_of(" \t", equal + 1);
            if (begin == std::string::npos) { result.error = "Empty boot path in SYSTEM.CNF"; return result; }
            auto end = config.find_first_of("\r\n; \t", begin);
            result.boot_path = config.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
            while (!result.boot_path.empty() && (result.boot_path.front() == '"' || result.boot_path.front() == '\'')) result.boot_path.erase(result.boot_path.begin());
            while (!result.boot_path.empty() && (result.boot_path.back() == '"' || result.boot_path.back() == '\'')) result.boot_path.pop_back();
            std::string file_name = result.boot_path;
            const auto colon = file_name.find(':');
            if (colon != std::string::npos) file_name = file_name.substr(colon + 1);
            while (!file_name.empty() && (file_name.front() == '/' || file_name.front() == '\\')) file_name.erase(file_name.begin());
            const auto slash = file_name.find_last_of("/\\");
            if (slash != std::string::npos) file_name = file_name.substr(slash + 1);
            for (const auto& candidate : root) {
                if (!candidate.directory && candidate.name == file_name) {
                    result.found = true; result.executable_lba = candidate.lba; result.executable_size = candidate.byte_size;
                    result.error.clear(); return result;
                }
            }
            result.error = "Boot executable was not found in the ISO root directory";
            return result;
        }
    }
    result.error = "SYSTEM.CNF not found in the ISO root directory";
    return result;
}
} // namespace retroshelf::media
