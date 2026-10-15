#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace retroshelf {

// Testable RGBA8 output surface for the future GS rasterizer.
// This is a host-side framebuffer, not yet a full PS2 GS implementation.
class GsFramebuffer {
public:
    struct Pixel {
        std::uint8_t r{0};
        std::uint8_t g{0};
        std::uint8_t b{0};
        std::uint8_t a{0};

        bool operator==(const Pixel& other) const {
            return r == other.r && g == other.g && b == other.b && a == other.a;
        }
        bool operator!=(const Pixel& other) const { return !(*this == other); }
    };

    // Rejects zero dimensions and dimensions that overflow allocation sizing.
    GsFramebuffer(std::size_t width, std::size_t height);

    std::size_t width() const;
    std::size_t height() const;
    void reset_scissor();
    // Right and bottom are exclusive, matching common rectangle APIs.
    void set_scissor(std::size_t left, std::size_t top,
                     std::size_t right, std::size_t bottom);
    void clear(Pixel color);
    bool put_pixel(std::size_t x, std::size_t y, Pixel color);
    bool get_pixel(std::size_t x, std::size_t y, Pixel& color) const;
    // Returns row-major RGBA bytes suitable for a later iOS texture upload.
    std::vector<std::uint8_t> rgba_bytes() const;

private:
    std::size_t width_;
    std::size_t height_;
    std::size_t left_{0};
    std::size_t top_{0};
    std::size_t right_;
    std::size_t bottom_;
    std::vector<Pixel> pixels_;
};

} // namespace retroshelf
