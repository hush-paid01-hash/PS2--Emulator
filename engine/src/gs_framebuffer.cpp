#include "retroshelf/gs_framebuffer.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace retroshelf {
namespace {
double edge(double ax, double ay, double bx, double by, double px, double py) {
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}
std::uint8_t channel(double value) {
    return static_cast<std::uint8_t>(std::clamp(std::lround(value), 0L, 255L));
}
}

GsFramebuffer::GsFramebuffer(std::size_t width, std::size_t height)
    : width_(width), height_(height), right_(width), bottom_(height) {
    if (width == 0 || height == 0 ||
        width > std::numeric_limits<std::size_t>::max() / height ||
        width * height > std::numeric_limits<std::size_t>::max() / sizeof(Pixel)) {
        throw std::invalid_argument("framebuffer dimensions are zero or overflow allocation sizing");
    }
    pixels_.resize(width * height);
}
std::size_t GsFramebuffer::width() const { return width_; }
std::size_t GsFramebuffer::height() const { return height_; }
void GsFramebuffer::reset_scissor() { left_ = 0; top_ = 0; right_ = width_; bottom_ = height_; }
void GsFramebuffer::set_scissor(std::size_t left, std::size_t top,
                                std::size_t right, std::size_t bottom) {
    left_ = std::min(left, width_); top_ = std::min(top, height_);
    right_ = std::min(right, width_); bottom_ = std::min(bottom, height_);
    if (right_ < left_) right_ = left_;
    if (bottom_ < top_) bottom_ = top_;
}
void GsFramebuffer::clear(Pixel color) {
    for (std::size_t y = top_; y < bottom_; ++y)
        for (std::size_t x = left_; x < right_; ++x)
            pixels_[y * width_ + x] = color;
}
bool GsFramebuffer::put_pixel(std::size_t x, std::size_t y, Pixel color) {
    if (x < left_ || x >= right_ || y < top_ || y >= bottom_) return false;
    pixels_[y * width_ + x] = color;
    return true;
}
bool GsFramebuffer::get_pixel(std::size_t x, std::size_t y, Pixel& color) const {
    if (x >= width_ || y >= height_) return false;
    color = pixels_[y * width_ + x];
    return true;
}

bool GsFramebuffer::draw_triangle(const Vertex& a, const Vertex& b, const Vertex& c) {
    const Vertex vertices[] = {a, b, c};
    for (const Vertex& v : vertices) {
        if (!std::isfinite(v.x) || !std::isfinite(v.y)) return false;
    }
    const double area = edge(a.x, a.y, b.x, b.y, c.x, c.y);
    if (!std::isfinite(area) || std::abs(area) < 1e-12) return false;

    const double minXd = std::floor(std::min({a.x, b.x, c.x}));
    const double maxXd = std::ceil(std::max({a.x, b.x, c.x}));
    const double minYd = std::floor(std::min({a.y, b.y, c.y}));
    const double maxYd = std::ceil(std::max({a.y, b.y, c.y}));
    const double maxWidth = static_cast<double>(width_);
    const double maxHeight = static_cast<double>(height_);
    if (maxXd < 0 || maxYd < 0 || minXd >= maxWidth || minYd >= maxHeight) return true;

    const auto clampCoord = [](double v, std::size_t limit) {
        if (v <= 0.0) return std::size_t{0};
        if (v >= static_cast<double>(limit)) return limit;
        return static_cast<std::size_t>(v);
    };
    const std::size_t x0 = std::max(left_, clampCoord(minXd, width_));
    const std::size_t y0 = std::max(top_, clampCoord(minYd, height_));
    const std::size_t x1 = std::min(right_, clampCoord(maxXd, width_));
    const std::size_t y1 = std::min(bottom_, clampCoord(maxYd, height_));

    for (std::size_t y = y0; y < y1; ++y) {
        for (std::size_t x = x0; x < x1; ++x) {
            const double px = static_cast<double>(x) + 0.5;
            const double py = static_cast<double>(y) + 0.5;
            const double wa = edge(b.x, b.y, c.x, c.y, px, py) / area;
            const double wb = edge(c.x, c.y, a.x, a.y, px, py) / area;
            const double wc = edge(a.x, a.y, b.x, b.y, px, py) / area;
            if (wa < 0.0 || wb < 0.0 || wc < 0.0) continue;
            pixels_[y * width_ + x] = Pixel{
                channel(wa * a.color.r + wb * b.color.r + wc * c.color.r),
                channel(wa * a.color.g + wb * b.color.g + wc * c.color.g),
                channel(wa * a.color.b + wb * b.color.b + wc * c.color.b),
                channel(wa * a.color.a + wb * b.color.a + wc * c.color.a)};
        }
    }
    return true;
}

std::vector<std::uint8_t> GsFramebuffer::rgba_bytes() const {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(pixels_.size() * 4);
    for (const Pixel& pixel : pixels_) {
        bytes.push_back(pixel.r); bytes.push_back(pixel.g);
        bytes.push_back(pixel.b); bytes.push_back(pixel.a);
    }
    return bytes;
}
} // namespace retroshelf
