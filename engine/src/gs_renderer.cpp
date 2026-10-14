#include "retroshelf/gs_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>

namespace retroshelf::gs {
namespace {
float edge(const ScreenVertex& a, const ScreenVertex& b, float x, float y) {
    return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
}
std::uint32_t mix_color(std::uint32_t a, std::uint32_t b, std::uint32_t c,
                        float wa, float wb, float wc) {
    std::uint32_t out = 0;
    for (unsigned shift : {24U, 16U, 8U, 0U}) {
        const float value = static_cast<float>((a >> shift) & 255U) * wa +
                            static_cast<float>((b >> shift) & 255U) * wb +
                            static_cast<float>((c >> shift) & 255U) * wc;
        out |= static_cast<std::uint32_t>(std::clamp(value, 0.0F, 255.0F) + 0.5F) << shift;
    }
    return out;
}
}

SoftwareRenderer::SoftwareRenderer(std::uint32_t width, std::uint32_t height)
    : width_(width), height_(height), pixels_() {
    const std::uint64_t count = static_cast<std::uint64_t>(width) * height;
    if (width == 0 || height == 0 || count > 16U * 1024U * 1024U) {
        width_ = height_ = 0;
        return;
    }
    pixels_.resize(static_cast<std::size_t>(count));
    clear();
}

void SoftwareRenderer::clear(std::uint32_t rgba) {
    std::fill(pixels_.begin(), pixels_.end(), rgba);
}

bool SoftwareRenderer::draw_triangle(const ScreenVertex& a, const ScreenVertex& b,
                                     const ScreenVertex& c) {
    if (width_ == 0 || height_ == 0 || pixels_.empty()) return false;
    const float area = edge(a, b, c.x, c.y);
    if (!std::isfinite(area) || std::fabs(area) < 0.0001F) return false;
    const float min_x = std::min({a.x, b.x, c.x});
    const float max_x = std::max({a.x, b.x, c.x});
    const float min_y = std::min({a.y, b.y, c.y});
    const float max_y = std::max({a.y, b.y, c.y});
    if (!std::isfinite(min_x) || !std::isfinite(max_x) ||
        !std::isfinite(min_y) || !std::isfinite(max_y)) return false;
    const int x0 = std::max(0, static_cast<int>(std::floor(std::max(-1.0F, min_x))));
    const int y0 = std::max(0, static_cast<int>(std::floor(std::max(-1.0F, min_y))));
    const int x1 = std::min(static_cast<int>(width_) - 1,
                            static_cast<int>(std::ceil(std::min(max_x, static_cast<float>(width_)))));
    const int y1 = std::min(static_cast<int>(height_) - 1,
                            static_cast<int>(std::ceil(std::min(max_y, static_cast<float>(height_)))));
    if (x0 > x1 || y0 > y1) return true;

    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const float px = static_cast<float>(x) + 0.5F;
            const float py = static_cast<float>(y) + 0.5F;
            const float ea = edge(b, c, px, py);
            const float eb = edge(c, a, px, py);
            const float ec = edge(a, b, px, py);
            if ((area > 0.0F && ea >= 0.0F && eb >= 0.0F && ec >= 0.0F) ||
                (area < 0.0F && ea <= 0.0F && eb <= 0.0F && ec <= 0.0F)) {
                const float wa = ea / area;
                const float wb = eb / area;
                const float wc = ec / area;
                pixels_[static_cast<std::size_t>(y) * width_ + static_cast<std::size_t>(x)] =
                    mix_color(a.rgba, b.rgba, c.rgba, wa, wb, wc);
            }
        }
    }
    return true;
}

bool SoftwareRenderer::write_ppm(const char* path) const {
    if (path == nullptr || width_ == 0 || height_ == 0 || pixels_.empty()) return false;
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out << "P6\n" << width_ << ' ' << height_ << "\n255\n";
    for (const auto pixel : pixels_) {
        const char rgb[] = {static_cast<char>((pixel >> 24U) & 255U),
                            static_cast<char>((pixel >> 16U) & 255U),
                            static_cast<char>((pixel >> 8U) & 255U)};
        out.write(rgb, sizeof(rgb));
    }
    return static_cast<bool>(out);
}

} // namespace retroshelf::gs
