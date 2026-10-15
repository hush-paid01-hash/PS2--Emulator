#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace retroshelf {

// Deterministic host-side RGBA surface and simple software triangle rasterizer.
// This is a rendering foundation, not the complete PS2 GS.
class GsFramebuffer {
public:
    struct Pixel {
        std::uint8_t r{0}, g{0}, b{0}, a{0};
        bool operator==(const Pixel& other) const {
            return r == other.r && g == other.g && b == other.b && a == other.a;
        }
        bool operator!=(const Pixel& other) const { return !(*this == other); }
    };
    struct Vertex {
        double x{0.0};
        double y{0.0};
        Pixel color{};
    };

    GsFramebuffer(std::size_t width, std::size_t height);
    std::size_t width() const;
    std::size_t height() const;
    void reset_scissor();
    void set_scissor(std::size_t left, std::size_t top,
                     std::size_t right, std::size_t bottom);
    void clear(Pixel color);
    bool put_pixel(std::size_t x, std::size_t y, Pixel color);
    bool get_pixel(std::size_t x, std::size_t y, Pixel& color) const;
    // Rasterizes a filled triangle at pixel centers with interpolated RGBA.
    // Returns false for degenerate/non-finite triangles.
    bool draw_triangle(const Vertex& a, const Vertex& b, const Vertex& c);
    std::vector<std::uint8_t> rgba_bytes() const;

private:
    std::size_t width_;
    std::size_t height_;
    std::size_t left_{0}, top_{0}, right_, bottom_;
    std::vector<Pixel> pixels_;
};

} // namespace retroshelf
