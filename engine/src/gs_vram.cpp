#include "retroshelf/gs_vram.hpp"
#include <algorithm>

namespace retroshelf::gs {

GsVram::GsVram() : bytes_(Size, 0) {}
void GsVram::reset() { std::fill(bytes_.begin(), bytes_.end(), 0); }

bool GsVram::read32(std::size_t byte_offset, std::uint32_t& value) const {
    if ((byte_offset & 3U) != 0U || byte_offset > Size - 4U) return false;
    value = static_cast<std::uint32_t>(bytes_[byte_offset]) |
            (static_cast<std::uint32_t>(bytes_[byte_offset + 1U]) << 8U) |
            (static_cast<std::uint32_t>(bytes_[byte_offset + 2U]) << 16U) |
            (static_cast<std::uint32_t>(bytes_[byte_offset + 3U]) << 24U);
    return true;
}

bool GsVram::write32(std::size_t byte_offset, std::uint32_t value) {
    if ((byte_offset & 3U) != 0U || byte_offset > Size - 4U) return false;
    for (unsigned lane = 0; lane < 4; ++lane)
        bytes_[byte_offset + lane] = static_cast<std::uint8_t>(value >> (lane * 8U));
    return true;
}

bool GsVram::read_pixel_ct32(std::uint32_t x, std::uint32_t y, std::uint32_t width,
                             std::uint32_t& rgba) const {
    if (width == 0 || x >= width) return false;
    const auto pixel = static_cast<std::uint64_t>(y) * width + x;
    const auto offset = pixel * 4U;
    if (offset > Size - 4U) return false;
    return read32(static_cast<std::size_t>(offset), rgba);
}

bool GsVram::write_pixel_ct32(std::uint32_t x, std::uint32_t y, std::uint32_t width,
                              std::uint32_t rgba) {
    if (width == 0 || x >= width) return false;
    const auto pixel = static_cast<std::uint64_t>(y) * width + x;
    const auto offset = pixel * 4U;
    if (offset > Size - 4U) return false;
    return write32(static_cast<std::size_t>(offset), rgba);
}

const std::vector<std::uint8_t>& GsVram::bytes() const { return bytes_; }

} // namespace retroshelf::gs
