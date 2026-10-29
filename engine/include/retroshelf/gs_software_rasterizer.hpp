#pragma once

#include "retroshelf/gs_framebuffer.hpp"

#include <cstdint>

namespace retroshelf {

// Generic software rasterizer for renderer bring-up; not a complete PS2 GS.
class GsSoftwareRasterizer {
public:
    struct Point { std::int32_t x{0}; std::int32_t y{0}; };

    bool draw_line(GsFramebuffer& target, Point a, Point b,
                   GsFramebuffer::Rgba8 color) const;
    // Interpolates RGBA from the first endpoint to the second endpoint.
    bool draw_line_gouraud(GsFramebuffer& target, Point a, Point b,
                           GsFramebuffer::Rgba8 color_a,
                           GsFramebuffer::Rgba8 color_b) const;
    bool draw_triangle(GsFramebuffer& target, Point a, Point b, Point c,
                       GsFramebuffer::Rgba8 color) const;
    bool draw_triangle_gouraud(GsFramebuffer& target, Point a, Point b, Point c,
                               GsFramebuffer::Rgba8 color_a,
                               GsFramebuffer::Rgba8 color_b,
                               GsFramebuffer::Rgba8 color_c) const;
};

} // namespace retroshelf
