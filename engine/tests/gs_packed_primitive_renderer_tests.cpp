#include "retroshelf/gs_packed_primitive_renderer.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using retroshelf::GsFramebuffer;
using retroshelf::GsPackedPrimitiveRenderer;
using Write = retroshelf::GsRegisterDecoder::Write;

static std::uint64_t xyz(std::uint16_t x, std::uint16_t y) {
    return (static_cast<std::uint64_t>(y) << 16U) |
           static_cast<std::uint64_t>(x);
}

int main() {
    GsFramebuffer framebuffer;
    assert(framebuffer.resize(8, 8));
    GsPackedPrimitiveRenderer renderer;
    const std::vector<Write> writes{
        {0x0, 0x3},                         // PRIM: triangle list
        {0x1, 0xFF0000FFULL},              // RGBA: red, opaque
        {0x5, xyz(1U << 4U, 1U << 4U)},
        {0x5, xyz(6U << 4U, 1U << 4U)},
        {0x5, xyz(1U << 4U, 6U << 4U)}
    };
    assert(renderer.consume(writes, framebuffer));
    GsFramebuffer::Rgba8 pixel{};
    assert(framebuffer.get_pixel(2, 2, pixel));
    assert(pixel.r == 255 && pixel.g == 0 && pixel.b == 0 && pixel.a == 255);
    GsFramebuffer::Rgba8 untouched{};
    assert(framebuffer.get_pixel(7, 7, untouched));
    assert(untouched.r == 0 && untouched.g == 0 && untouched.b == 0);

    renderer.reset();
    framebuffer.clear();
    assert(renderer.consume({{0x0, 0x0}, {0x1, 0xFFFFFFFFULL},
                             {0x5, xyz(1U << 4U, 1U << 4U)},
                             {0x5, xyz(6U << 4U, 1U << 4U)},
                             {0x5, xyz(1U << 4U, 6U << 4U)}}, framebuffer));
    GsFramebuffer::Rgba8 ignored{};
    assert(framebuffer.get_pixel(2, 2, ignored));
    assert(ignored.r == 0 && ignored.g == 0 && ignored.b == 0);

    renderer.reset();
    framebuffer.clear();
    // A GS sprite uses two XYZ2 vertices as opposite rectangle corners.
    assert(renderer.consume({{0x0, 0x6}, {0x1, 0xFF00FF00ULL},
                             {0x5, xyz(2U << 4U, 2U << 4U)},
                             {0x5, xyz(5U << 4U, 5U << 4U)}}, framebuffer));
    GsFramebuffer::Rgba8 sprite_pixel{};
    assert(framebuffer.get_pixel(3, 3, sprite_pixel));
    assert(sprite_pixel.r == 0 && sprite_pixel.g == 255 &&
           sprite_pixel.b == 0 && sprite_pixel.a == 255);
    GsFramebuffer::Rgba8 sprite_outside{};
    assert(framebuffer.get_pixel(5, 5, sprite_outside));
    assert(sprite_outside.r == 0 && sprite_outside.g == 0 &&
           sprite_outside.b == 0 && sprite_outside.a == 0);

    renderer.reset();
    framebuffer.clear();
    // A PRIM change discards an incomplete sprite instead of leaking vertices.
    assert(renderer.consume({{0x0, 0x6}, {0x1, 0xFFFFFFFFULL},
                             {0x5, xyz(1U << 4U, 1U << 4U)},
                             {0x0, 0x0}, {0x0, 0x6},
                             {0x5, xyz(2U << 4U, 2U << 4U)}}, framebuffer));
    GsFramebuffer::Rgba8 incomplete{};
    assert(framebuffer.get_pixel(1, 1, incomplete));
    assert(incomplete.r == 0 && incomplete.g == 0 &&
           incomplete.b == 0 && incomplete.a == 0);

    renderer.reset();
    framebuffer.clear();
    // A line-list primitive draws two XYZ2 endpoints in the current colour.
    assert(renderer.consume({{0x0, 0x1}, {0x1, 0xFFFF0000ULL},
                             {0x5, xyz(1U << 4U, 1U << 4U)},
                             {0x5, xyz(5U << 4U, 5U << 4U)}}, framebuffer));
    GsFramebuffer::Rgba8 line_pixel{};
    assert(framebuffer.get_pixel(3, 3, line_pixel));
    assert(line_pixel.r == 0 && line_pixel.g == 0 &&
           line_pixel.b == 255 && line_pixel.a == 255);
    GsFramebuffer::Rgba8 line_off_path{};
    assert(framebuffer.get_pixel(3, 2, line_off_path));
    assert(line_off_path.r == 0 && line_off_path.g == 0 &&
           line_off_path.b == 0 && line_off_path.a == 0);

    renderer.reset();
    framebuffer.clear();
    // XYZ2 X/Y are signed 12.4 coordinates: 0xFFF0 represents -1.0.
    // The sprite clips at the visible edge instead of treating -1 as 4095.
    assert(renderer.consume({{0x0, 0x6}, {0x1, 0xFF0000FFULL},
                             {0x5, xyz(0xFFF0U, 0xFFF0U)},
                             {0x5, xyz(5U << 4U, 5U << 4U)}}, framebuffer));
    GsFramebuffer::Rgba8 signed_coordinate_pixel{};
    assert(framebuffer.get_pixel(3, 3, signed_coordinate_pixel));
    assert(signed_coordinate_pixel.r == 255 && signed_coordinate_pixel.g == 0 &&
           signed_coordinate_pixel.b == 0 && signed_coordinate_pixel.a == 255);
    GsFramebuffer::Rgba8 signed_coordinate_outside{};
    assert(framebuffer.get_pixel(6, 6, signed_coordinate_outside));
    assert(signed_coordinate_outside.r == 0 && signed_coordinate_outside.g == 0 &&
           signed_coordinate_outside.b == 0 && signed_coordinate_outside.a == 0);

    renderer.reset();
    framebuffer.clear();
    // A triangle strip emits a new triangle for each vertex after the first two.
    assert(renderer.consume({{0x0, 0x4}, {0x1, 0xFF0000FFULL},
                             {0x5, xyz(1U << 4U, 1U << 4U)},
                             {0x5, xyz(6U << 4U, 1U << 4U)},
                             {0x5, xyz(1U << 4U, 6U << 4U)},
                             {0x5, xyz(6U << 4U, 6U << 4U)}}, framebuffer));
    GsFramebuffer::Rgba8 strip_pixel{};
    assert(framebuffer.get_pixel(4, 4, strip_pixel));
    assert(strip_pixel.r == 255 && strip_pixel.g == 0 &&
           strip_pixel.b == 0 && strip_pixel.a == 255);

    renderer.reset();
    framebuffer.clear();
    // A triangle fan keeps the first vertex as the common center.
    assert(renderer.consume({{0x0, 0x5}, {0x1, 0xFF00FF00ULL},
                             {0x5, xyz(3U << 4U, 3U << 4U)},
                             {0x5, xyz(1U << 4U, 1U << 4U)},
                             {0x5, xyz(6U << 4U, 1U << 4U)},
                             {0x5, xyz(6U << 4U, 6U << 4U)},
                             {0x5, xyz(1U << 4U, 6U << 4U)}}, framebuffer));
    GsFramebuffer::Rgba8 fan_pixel{};
    assert(framebuffer.get_pixel(4, 4, fan_pixel));
    assert(fan_pixel.r == 0 && fan_pixel.g == 255 &&
           fan_pixel.b == 0 && fan_pixel.a == 255);


    renderer.reset();
    framebuffer.clear();
    // PRIM.IIP enables Gouraud shading: vertex colours interpolate across the face.
    assert(renderer.consume({{0x0, 0xBU},
                             {0x1, 0xFF0000FFULL},
                             {0x5, xyz(1U << 4U, 1U << 4U)},
                             {0x1, 0xFF00FF00ULL},
                             {0x5, xyz(6U << 4U, 1U << 4U)},
                             {0x1, 0xFFFF0000ULL},
                             {0x5, xyz(1U << 4U, 6U << 4U)}}, framebuffer));
    GsFramebuffer::Rgba8 interpolated{};
    assert(framebuffer.get_pixel(2, 2, interpolated));
    assert(interpolated.r > 0 && interpolated.g > 0 && interpolated.b > 0);
    assert(interpolated.a == 255);

    std::cout << "GS packed primitive renderer tests passed\n";
    return 0;
}
