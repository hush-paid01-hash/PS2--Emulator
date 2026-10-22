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

    std::cout << "GS packed primitive renderer tests passed\n";
    return 0;
}
