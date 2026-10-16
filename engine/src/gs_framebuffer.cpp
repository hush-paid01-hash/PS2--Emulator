#include "retroshelf/gs_framebuffer.hpp"

#include <limits>

namespace retroshelf {

bool GsFramebuffer::resize(std::size_t width, std::size_t height) {
    if (width == 0 || height == 0 || width > MaxDimension || height > MaxDimension)
        return false;
    if (width > std::numeric_limits<std::size_t>::max() / height)
        return false;
    const std::size_t count = width * height;
    if (count > MaxPixels || count > std::numeric_limits<std::size_t>::max() / 4U)
        return false;

    std::vector<std::uint8_t> replacement(count * 4U, 0);
    pixels_.swap(replacement);
    width_ = width;
    height_ = height;
    return true;
}

void GsFramebuffer::clear() {
    clear(Rgba8{});
}

void GsFramebuffer::clear(Rgba8 color) {
    for (std::size_t i = 0; i + 3U < pixels_.size(); i += 4U) {
        pixels_[i] = color.r;
        pixels_[i + 1U] = color.g;
        pixels_[i + 2U] = color.b;
        pixels_[i + 3U] = color.a;
    }
}

bool GsFramebuffer::set_pixel(std::size_t x, std::size_t y, Rgba8 color) {
    if (x >= width_ || y >= height_) return false;
    const std::size_t offset = (y * width_ + x) * 4U;
    pixels_[offset] = color.r;
    pixels_[offset + 1U] = color.g;
    pixels_[offset + 2U] = color.b;
    pixels_[offset + 3U] = color.a;
    return true;
}

bool GsFramebuffer::get_pixel(std::size_t x, std::size_t y, Rgba8& color) const {
    if (x >= width_ || y >= height_) return false;
    const std::size_t offset = (y * width_ + x) * 4U;
    color = {pixels_[offset], pixels_[offset + 1U],
             pixels_[offset + 2U], pixels_[offset + 3U]};
    return true;
}

std::size_t GsFramebuffer::width() const { return width_; }
std::size_t GsFramebuffer::height() const { return height_; }
bool GsFramebuffer::empty() const { return pixels_.empty(); }
const std::vector<std::uint8_t>& GsFramebuffer::rgba_bytes() const { return pixels_; }

} // namespace retroshelf
