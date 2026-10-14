#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace retroshelf::gs {

// Minimal 4 MiB GS VRAM storage primitive. This is not yet the PS2's
// swizzled pixel-addressing or GIF/primitive rasterization implementation.
class GsVram {
public:
    static constexpr std::size_t Size = 4U * 1024U * 1024U;
    GsVram();
    void reset();
    bool read32(std::size_t byte_offset, std::uint32_t& value) const;
    bool write32(std::size_t byte_offset, std::uint32_t value);
    bool read_pixel_ct32(std::uint32_t x, std::uint32_t y, std::uint32_t width,
                         std::uint32_t& rgba) const;
    bool write_pixel_ct32(std::uint32_t x, std::uint32_t y, std::uint32_t width,
                          std::uint32_t rgba);
    const std::vector<std::uint8_t>& bytes() const;
private:
    std::vector<std::uint8_t> bytes_;
};

} // namespace retroshelf::gs
