#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace retroshelf::gs {

struct ScreenVertex {
    float x = 0.0F;
    float y = 0.0F;
    std::uint32_t rgba = 0xFFFFFFFFU; // 0xRRGGBBAA
};

// Small deterministic software framebuffer used as the first rendering target.
// This is a renderer foundation, not yet a cycle-accurate PS2 GS implementation.
class SoftwareRenderer {
public:
    SoftwareRenderer(std::uint32_t width, std::uint32_t height);
    std::uint32_t width() const { return width_; }
    std::uint32_t height() const { return height_; }
    void clear(std::uint32_t rgba = 0x000000FFU);
    bool draw_triangle(const ScreenVertex& a, const ScreenVertex& b,
                       const ScreenVertex& c);
    const std::vector<std::uint32_t>& pixels() const { return pixels_; }
    bool write_ppm(const char* path) const;

private:
    std::uint32_t width_;
    std::uint32_t height_;
    std::vector<std::uint32_t> pixels_;
};

} // namespace retroshelf::gs
