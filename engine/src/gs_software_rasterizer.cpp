#include "retroshelf/gs_software_rasterizer.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>

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

bool GsSoftwareRasterizer::draw_line(GsFramebuffer& target, Point a, Point b,
                                     GsFramebuffer::Rgba8 color) const {
    if (target.empty() || !safe_point(a) || !safe_point(b)) return false;

    std::int64_t x = a.x;
    std::int64_t y = a.y;
    const std::int64_t dx = std::abs(static_cast<std::int64_t>(b.x) - a.x);
    const std::int64_t dy = std::abs(static_cast<std::int64_t>(b.y) - a.y);
    const std::int64_t sx = a.x < b.x ? 1 : -1;
    const std::int64_t sy = a.y < b.y ? 1 : -1;
    std::int64_t error = dx - dy;

    for (;;) {
        if (x >= 0 && y >= 0 && x < static_cast<std::int64_t>(target.width()) &&
            y < static_cast<std::int64_t>(target.height())) {
            target.set_pixel(static_cast<std::size_t>(x), static_cast<std::size_t>(y), color);
        }
        if (x == b.x && y == b.y) break;
        const std::int64_t doubled_error = 2 * error;
        if (doubled_error > -dy) { error -= dy; x += sx; }
        if (doubled_error < dx) { error += dx; y += sy; }
    }
    return true;
}

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


bool GsSoftwareRasterizer::draw_triangle_gouraud(
    GsFramebuffer& target, Point a, Point b, Point c,
    GsFramebuffer::Rgba8 color_a, GsFramebuffer::Rgba8 color_b,
    GsFramebuffer::Rgba8 color_c) const {
    if (target.empty() || !safe_point(a) || !safe_point(b) || !safe_point(c))
        return false;

    const std::int64_t area = edge(a, b, c.x, c.y);
    if (area == 0) return true;
    const std::int64_t min_x = std::max<std::int64_t>(0, std::min({a.x, b.x, c.x}));
    const std::int64_t min_y = std::max<std::int64_t>(0, std::min({a.y, b.y, c.y}));
    const std::int64_t max_x = std::min<std::int64_t>(
        static_cast<std::int64_t>(target.width()) - 1, std::max({a.x, b.x, c.x}));
    const std::int64_t max_y = std::min<std::int64_t>(
        static_cast<std::int64_t>(target.height()) - 1, std::max({a.y, b.y, c.y}));
    if (min_x > max_x || min_y > max_y) return true;

    const bool positive = area > 0;
    const std::int64_t denominator = positive ? area : -area;
    const auto interpolate = [denominator](
        std::int64_t wa, std::int64_t wb, std::int64_t wc,
        std::uint8_t ca, std::uint8_t cb, std::uint8_t cc) {
        const std::int64_t value = (wa * ca + wb * cb + wc * cc) / denominator;
        return static_cast<std::uint8_t>(std::clamp<std::int64_t>(value, 0, 255));
    };

    for (std::int64_t y = min_y; y <= max_y; ++y) {
        for (std::int64_t x = min_x; x <= max_x; ++x) {
            std::int64_t wa = edge(b, c, x, y);
            std::int64_t wb = edge(c, a, x, y);
            std::int64_t wc = edge(a, b, x, y);
            if (positive ? (wa < 0 || wb < 0 || wc < 0)
                         : (wa > 0 || wb > 0 || wc > 0))
                continue;
            if (!positive) { wa = -wa; wb = -wb; wc = -wc; }
            target.set_pixel(static_cast<std::size_t>(x), static_cast<std::size_t>(y), {
                interpolate(wa, wb, wc, color_a.r, color_b.r, color_c.r),
                interpolate(wa, wb, wc, color_a.g, color_b.g, color_c.g),
                interpolate(wa, wb, wc, color_a.b, color_b.b, color_c.b),
                interpolate(wa, wb, wc, color_a.a, color_b.a, color_c.a)
            });
        }
    }
    return true;
}

} // namespace retroshelf
