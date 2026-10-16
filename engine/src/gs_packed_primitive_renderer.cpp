#include "retroshelf/gs_packed_primitive_renderer.hpp"

namespace retroshelf {
namespace {
constexpr std::uint8_t PrimRegister = 0x0;
constexpr std::uint8_t RgbaqRegister = 0x1;
constexpr std::uint8_t Xyz2Register = 0x5;
constexpr std::uint64_t PrimitiveMask = 0x7U;
constexpr std::uint64_t TrianglePrimitive = 0x3U;
constexpr std::int32_t MaxCoordinate = 1000000;
}

bool GsPackedPrimitiveRenderer::consume(
    const std::vector<GsRegisterDecoder::Write>& writes, GsFramebuffer& target) {
    error_.clear();
    for (const auto& write : writes) {
        switch (write.address) {
        case PrimRegister:
            prim_ = write.value;
            if ((prim_ & PrimitiveMask) != TrianglePrimitive)
                triangle_vertices_.clear();
            break;
        case RgbaqRegister:
            color_.r = static_cast<std::uint8_t>(write.value & 0xFFU);
            color_.g = static_cast<std::uint8_t>((write.value >> 8U) & 0xFFU);
            color_.b = static_cast<std::uint8_t>((write.value >> 16U) & 0xFFU);
            color_.a = static_cast<std::uint8_t>((write.value >> 24U) & 0xFFU);
            break;
        case Xyz2Register:
            if ((prim_ & PrimitiveMask) != TrianglePrimitive)
                break;
            {
                // GS XYZ2 coordinates are unsigned 12.4 fixed-point values.
                const auto raw_x = static_cast<std::uint32_t>(write.value & 0xFFFFU);
                const auto raw_y = static_cast<std::uint32_t>((write.value >> 16U) & 0xFFFFU);
                const auto x = static_cast<std::int32_t>(raw_x >> 4U);
                const auto y = static_cast<std::int32_t>(raw_y >> 4U);
                if (x > MaxCoordinate || y > MaxCoordinate) {
                    error_ = "GS XYZ2 coordinate exceeds supported range";
                    return false;
                }
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
    triangle_vertices_.clear();
    error_.clear();
}

} // namespace retroshelf
