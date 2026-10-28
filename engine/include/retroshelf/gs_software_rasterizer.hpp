#pragma once

#include "retroshelf/gs_framebuffer.hpp"

#include <cstdint>

namespace retroshelf {

// Generic, flat-colour triangle rasterizer for renderer bring-up.
// It is not yet a PS2 GS primitive decoder or a complete GS renderer.
class GsSoftwareRasterizer {
public:
    struct Point {
        std::int32_t x{0};
        std::int32_t y{0};
    };

    // Draws a flat-colour line, clipped to the target surface. Returns false for
    // an empty target or coordinates outside the supported safe range.
    bool draw_line(GsFramebuffer& target, Point a, Point b,
                   GsFramebuffer::Rgba8 color) const;

    // Draws a filled triangle, clipped to the target surface. Returns false for
    // an empty target or coordinates outside the supported safe range.
    bool draw_triangle(GsFramebuffer& target, Point a, Point b, Point c,
                       GsFramebuffer::Rgba8 color) const;
};

} // namespace retroshelf
