#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace retroshelf {

// Bounded software pixel surface for the future GS rasterizer.
// This stores pixels only; it does not yet interpret GS drawing commands.
class GsFramebuffer {
public:
    struct Rgba8 {
        std::uint8_t r{0};
        std::uint8_t g{0};
        std::uint8_t b{0};
        std::uint8_t a{0};
        bool operator==(const Rgba8& other) const {
            return r == other.r && g == other.g && b == other.b && a == other.a;
        }
        bool operator!=(const Rgba8& other) const { return !(*this == other); }
    };

    static constexpr std::size_t MaxDimension = 4096;
    static constexpr std::size_t MaxPixels = 16U * 1024U * 1024U;

    // Returns false for zero, oversized, or overflow-prone dimensions.
    bool resize(std::size_t width, std::size_t height);
    void clear();
    void clear(Rgba8 color);
    // Returns false when coordinates are outside the surface.
    bool set_pixel(std::size_t x, std::size_t y, Rgba8 color);
    bool get_pixel(std::size_t x, std::size_t y, Rgba8& color) const;

    std::size_t width() const;
    std::size_t height() const;
    bool empty() const;
    // Pixels are row-major RGBA8 bytes, suitable for a later upload/conversion layer.
    const std::vector<std::uint8_t>& rgba_bytes() const;

private:
    std::size_t width_{0};
    std::size_t height_{0};
    std::vector<std::uint8_t> pixels_;
};

} // namespace retroshelf
