#include "retroshelf/gs_framebuffer.hpp"

#include <limits>
#include <stdexcept>

namespace retroshelf {

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

void GsFramebuffer::reset_scissor() {
    left_ = 0;
    top_ = 0;
    right_ = width_;
    bottom_ = height_;
}

void GsFramebuffer::set_scissor(std::size_t left, std::size_t top,
                                std::size_t right, std::size_t bottom) {
    left_ = left < width_ ? left : width_;
    top_ = top < height_ ? top : height_;
    right_ = right < width_ ? right : width_;
    bottom_ = bottom < height_ ? bottom : height_;
    if (right_ < left_) right_ = left_;
    if (bottom_ < top_) bottom_ = top_;
}

void GsFramebuffer::clear(Pixel color) {
    for (std::size_t y = top_; y < bottom_; ++y) {
        for (std::size_t x = left_; x < right_; ++x) {
            pixels_[y * width_ + x] = color;
        }
    }
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

std::vector<std::uint8_t> GsFramebuffer::rgba_bytes() const {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(pixels_.size() * 4);
    for (const Pixel& pixel : pixels_) {
        bytes.push_back(pixel.r);
        bytes.push_back(pixel.g);
        bytes.push_back(pixel.b);
        bytes.push_back(pixel.a);
    }
    return bytes;
}

} // namespace retroshelf
