#include "retroshelf/gs_packed_primitive_renderer.hpp"

#include <algorithm>

namespace retroshelf {
namespace {
constexpr std::uint8_t PrimRegister = 0x0;
constexpr std::uint8_t RgbaqRegister = 0x1;
constexpr std::uint8_t Xyz2Register = 0x5;
constexpr std::uint64_t PrimitiveMask = 0x7U;
constexpr std::uint64_t LinePrimitive = 0x1U;
constexpr std::uint64_t TrianglePrimitive = 0x3U;
constexpr std::uint64_t SpritePrimitive = 0x6U;
constexpr std::int32_t MaxCoordinate = 1000000;

// GS XYZ2 X/Y fields are unsigned 16-bit 12.4 fixed-point coordinates.
// High-bit values are valid positions in the GS coordinate space, not negatives.
std::int32_t decode_xyz_coordinate(std::uint16_t raw) {
    return static_cast<std::int32_t>(raw >> 4U);
}
}

bool GsPackedPrimitiveRenderer::consume(
    const std::vector<GsRegisterDecoder::Write>& writes, GsFramebuffer& target) {
    error_.clear();
    for (const auto& write : writes) {
        switch (write.address) {
        case PrimRegister:
            prim_ = write.value;
            if ((prim_ & PrimitiveMask) != LinePrimitive)
                line_vertices_.clear();
            if ((prim_ & PrimitiveMask) != TrianglePrimitive)
                triangle_vertices_.clear();
            if ((prim_ & PrimitiveMask) != SpritePrimitive)
                sprite_vertices_.clear();
            break;
        case RgbaqRegister:
            color_.r = static_cast<std::uint8_t>(write.value & 0xFFU);
            color_.g = static_cast<std::uint8_t>((write.value >> 8U) & 0xFFU);
            color_.b = static_cast<std::uint8_t>((write.value >> 16U) & 0xFFU);
            color_.a = static_cast<std::uint8_t>((write.value >> 24U) & 0xFFU);
            break;
        case Xyz2Register:
            if ((prim_ & PrimitiveMask) != LinePrimitive &&
                (prim_ & PrimitiveMask) != TrianglePrimitive &&
                (prim_ & PrimitiveMask) != SpritePrimitive)
                break;
            {
                // GS XYZ2 coordinates are signed 16-bit 12.4 fixed-point values.
                const auto raw_x = static_cast<std::uint16_t>(write.value & 0xFFFFU);
                const auto raw_y = static_cast<std::uint16_t>((write.value >> 16U) & 0xFFFFU);
                const auto x = decode_xyz_coordinate(raw_x);
                const auto y = decode_xyz_coordinate(raw_y);
                if (x > MaxCoordinate || y > MaxCoordinate) {
                    error_ = "GS XYZ2 coordinate exceeds supported range";
                    return false;
                }
                if ((prim_ & PrimitiveMask) == LinePrimitive) {
                    line_vertices_.push_back({x, y});
                    if (line_vertices_.size() == 2U) {
                        if (!rasterizer_.draw_line(target, line_vertices_[0],
                                                   line_vertices_[1], color_)) {
                            error_ = "GS line could not be drawn to framebuffer";
                            return false;
                        }
                        line_vertices_.clear();
                    }
                } else if ((prim_ & PrimitiveMask) == SpritePrimitive) {
                    sprite_vertices_.push_back({x, y});
                    if (sprite_vertices_.size() == 2U) {
                        const auto left = std::max<std::int32_t>(
                            std::min(sprite_vertices_[0].x, sprite_vertices_[1].x), 0);
                        const auto top = std::max<std::int32_t>(
                            std::min(sprite_vertices_[0].y, sprite_vertices_[1].y), 0);
                        const auto right = std::min<std::int32_t>(
                            std::max(sprite_vertices_[0].x, sprite_vertices_[1].x),
                            static_cast<std::int32_t>(target.width()));
                        const auto bottom = std::min<std::int32_t>(
                            std::max(sprite_vertices_[0].y, sprite_vertices_[1].y),
                            static_cast<std::int32_t>(target.height()));
                        for (auto py = top; py < bottom; ++py) {
                            for (auto px = left; px < right; ++px) {
                                target.set_pixel(static_cast<std::size_t>(px),
                                                 static_cast<std::size_t>(py), color_);
                            }
                        }
                        sprite_vertices_.clear();
                    }
                } else {
                    triangle_vertices_.push_back({x, y});
                    if (triangle_vertices_.size() == 3U) {
                        if (!rasterizer_.draw_triangle(target, triangle_vertices_[0],
                                                       triangle_vertices_[1],
                                                       triangle_vertices_[2], color_)) {
                            error_ = "GS triangle could not be drawn to framebuffer";
                            return false;
                        }
                        triangle_vertices_.clear();
                    }
                }
            }
            break;
        default:
            break;
        }
    }
    return true;
}

const std::string& GsPackedPrimitiveRenderer::error() const { return error_; }

void GsPackedPrimitiveRenderer::reset() {
    prim_ = 0;
    color_ = {};
    line_vertices_.clear();
    triangle_vertices_.clear();
    sprite_vertices_.clear();
    error_.clear();
}

} // namespace retroshelf
