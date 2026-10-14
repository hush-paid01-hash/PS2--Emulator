#include "retroshelf/disc_image.hpp"

#include <array>
#include <cctype>
#include <fstream>
#include <limits>

namespace retroshelf::media {
namespace {
constexpr std::uint64_t SectorSize = 2048;
constexpr std::uint64_t PvdOffset = 16 * SectorSize;
constexpr std::size_t PvdSize = 2048;

std::uint16_t read_le16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0] | (static_cast<std::uint16_t>(p[1]) << 8U));
}
std::uint32_t read_le32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0])
        | (static_cast<std::uint32_t>(p[1]) << 8U)
        | (static_cast<std::uint32_t>(p[2]) << 16U)
        | (static_cast<std::uint32_t>(p[3]) << 24U);
}
}

DiscImageInfo inspect_iso9660(const std::string& path) {
    DiscImageInfo result;
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) { result.error = "Could not open image file"; return result; }
    const auto end = file.tellg();
    if (end < 0 || static_cast<std::uint64_t>(end) < PvdOffset + PvdSize) {
        result.error = "File is too small to contain an ISO-9660 volume descriptor";
        return result;
    }
    file.seekg(static_cast<std::streamoff>(PvdOffset), std::ios::beg);
    std::array<std::uint8_t, PvdSize> pvd{};
    if (!file.read(reinterpret_cast<char*>(pvd.data()), static_cast<std::streamsize>(pvd.size()))) {
        result.error = "Could not read ISO-9660 volume descriptor"; return result;
    }
    if (pvd[0] != 1 || pvd[1] != 'C' || pvd[2] != 'D' || pvd[3] != '0'
        || pvd[4] != '0' || pvd[5] != '1' || pvd[6] != 1) {
        result.error = "ISO-9660 primary volume descriptor signature not found at sector 16";
        return result;
    }
    const auto sectors_le = read_le32(pvd.data() + 80);
    const auto sectors_be = (static_cast<std::uint32_t>(pvd[84]) << 24U)
        | (static_cast<std::uint32_t>(pvd[85]) << 16U)
        | (static_cast<std::uint32_t>(pvd[86]) << 8U) | static_cast<std::uint32_t>(pvd[87]);
    const auto block_le = read_le16(pvd.data() + 128);
    const auto block_be = static_cast<std::uint16_t>((static_cast<std::uint16_t>(pvd[130]) << 8U) | pvd[131]);
    if (sectors_le == 0 || sectors_le != sectors_be || block_le != SectorSize || block_le != block_be) {
        result.error = "ISO-9660 volume size or logical block size is inconsistent"; return result;
    }
    const auto declared_bytes = static_cast<std::uint64_t>(sectors_le) * block_le;
    if (declared_bytes > static_cast<std::uint64_t>(end)) {
        result.error = "Image is shorter than its declared ISO-9660 volume size"; return result;
    }
    result.valid = true;
    result.volume_sectors = sectors_le;
    result.logical_block_size = block_le;
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
    if (!info.valid) {
        if (error) *error = info.error;
        return false;
    }
    if (lba >= info.volume_sectors) {
        if (error) *error = "Requested logical block is outside the ISO volume";
        return false;
    }
    const auto offset = static_cast<std::uint64_t>(lba) * SectorSize;
    if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
        if (error) *error = "Requested logical block offset is not representable";
        return false;
    }
    std::ifstream file(path, std::ios::binary);
    if (!file) { if (error) *error = "Could not open image file"; return false; }
    file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    std::vector<std::uint8_t> candidate(static_cast<std::size_t>(SectorSize));
    if (!file.read(reinterpret_cast<char*>(candidate.data()), static_cast<std::streamsize>(candidate.size()))) {
        if (error) *error = "Could not read requested ISO logical block";
        return false;
    }
    sector = std::move(candidate);
    if (error) error->clear();
    return true;
}
} // namespace retroshelf::media
