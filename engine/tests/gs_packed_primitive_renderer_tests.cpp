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
    const auto pixel = framebuffer.pixel(2, 2);
    assert(pixel.r == 255 && pixel.g == 0 && pixel.b == 0 && pixel.a == 255);
    const auto untouched = framebuffer.pixel(7, 7);
    assert(untouched.r == 0 && untouched.g == 0 && untouched.b == 0);

    renderer.reset();
    framebuffer.clear();
    assert(renderer.consume({{0x0, 0x0}, {0x1, 0xFFFFFFFFULL},
                             {0x5, xyz(1U << 4U, 1U << 4U)},
                             {0x5, xyz(6U << 4U, 1U << 4U)},
                             {0x5, xyz(1U << 4U, 6U << 4U)}}, framebuffer));
    const auto ignored = framebuffer.pixel(2, 2);
    assert(ignored.r == 0 && ignored.g == 0 && ignored.b == 0);

    std::cout << "GS packed primitive renderer tests passed\n";
    return 0;
}
