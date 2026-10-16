#include "retroshelf/gs_software_rasterizer.hpp"

#include <algorithm>
#include <cstdint>

namespace retroshelf {
namespace {

constexpr std::int64_t MaxSafeCoordinate = 1000000;

std::int64_t edge(GsSoftwareRasterizer::Point a,
                  GsSoftwareRasterizer::Point b,
                  std::int64_t x, std::int64_t y) {
    return (static_cast<std::int64_t>(b.x) - a.x) * (y - a.y) -
           (static_cast<std::int64_t>(b.y) - a.y) * (x - a.x);
}

bool safe_point(GsSoftwareRasterizer::Point p) {
    return p.x >= -MaxSafeCoordinate && p.x <= MaxSafeCoordinate &&
           p.y >= -MaxSafeCoordinate && p.y <= MaxSafeCoordinate;
}

} // namespace

bool GsSoftwareRasterizer::draw_triangle(GsFramebuffer& target, Point a, Point b,
                                         Point c, GsFramebuffer::Rgba8 color) const {
    if (target.empty() || !safe_point(a) || !safe_point(b) || !safe_point(c))
        return false;

    const std::int64_t area = edge(a, b, c.x, c.y);
    if (area == 0) return true; // Degenerate triangles draw no pixels.

    const std::int64_t min_x = std::max<std::int64_t>(
        0, std::min({a.x, b.x, c.x}));
    const std::int64_t min_y = std::max<std::int64_t>(
        0, std::min({a.y, b.y, c.y}));
    const std::int64_t max_x = std::min<std::int64_t>(
        static_cast<std::int64_t>(target.width()) - 1,
        std::max({a.x, b.x, c.x}));
    const std::int64_t max_y = std::min<std::int64_t>(
        static_cast<std::int64_t>(target.height()) - 1,
        std::max({a.y, b.y, c.y}));
    if (min_x > max_x || min_y > max_y) return true;

    const bool positive = area > 0;
    for (std::int64_t y = min_y; y <= max_y; ++y) {
        for (std::int64_t x = min_x; x <= max_x; ++x) {
            const std::int64_t e0 = edge(a, b, x, y);
            const std::int64_t e1 = edge(b, c, x, y);
            const std::int64_t e2 = edge(c, a, x, y);
            if (positive ? (e0 >= 0 && e1 >= 0 && e2 >= 0)
                         : (e0 <= 0 && e1 <= 0 && e2 <= 0)) {
                target.set_pixel(static_cast<std::size_t>(x),
                                 static_cast<std::size_t>(y), color);
            }
        }
    }
    return true;
}

} // namespace retroshelf
