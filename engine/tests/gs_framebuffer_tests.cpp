#include "retroshelf/gs_framebuffer.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}
}

int main() {
    using retroshelf::GsFramebuffer;
    const GsFramebuffer::Pixel black{0, 0, 0, 255};
    const GsFramebuffer::Pixel red{255, 0, 0, 255};
    const GsFramebuffer::Pixel green{0, 255, 0, 255};
    GsFramebuffer frame(8, 8);

    expect(frame.width() == 8 && frame.height() == 8, "dimensions are retained");
    GsFramebuffer::Pixel pixel{};
    expect(frame.get_pixel(0, 0, pixel) && pixel == GsFramebuffer::Pixel{},
           "new pixels begin transparent black");
    frame.clear(black);
    frame.set_scissor(1, 0, 3, 1);
    expect(frame.put_pixel(1, 0, red), "pixel inside scissor is writable");
    expect(!frame.put_pixel(0, 0, green), "pixel outside scissor is rejected");
    frame.clear(green);
    expect(frame.get_pixel(0, 0, pixel) && pixel == black,
           "scissored clear leaves outside pixels unchanged");
    expect(frame.get_pixel(1, 0, pixel) && pixel == green,
           "scissored clear updates inside pixels");
    expect(!frame.get_pixel(8, 0, pixel), "out-of-range read is rejected");

    frame.reset_scissor();
    frame.clear(black);
    const GsFramebuffer::Vertex a{1.0, 1.0, red};
    const GsFramebuffer::Vertex b{7.0, 1.0, green};
    const GsFramebuffer::Vertex c{1.0, 7.0, GsFramebuffer::Pixel{0, 0, 255, 255}};
    expect(frame.draw_triangle(a, b, c), "non-degenerate triangle rasterizes");
    expect(frame.get_pixel(2, 2, pixel) && pixel != black,
           "triangle fills an interior pixel");
    expect(frame.get_pixel(6, 6, pixel) && pixel == black,
           "triangle does not fill pixels outside its edges");
    expect(frame.get_pixel(2, 2, pixel) && pixel.r > 0 && pixel.g > 0 && pixel.b > 0,
           "triangle interpolates vertex colors");

    frame.clear(black);
    expect(frame.draw_triangle(a, c, b), "reverse winding triangle rasterizes");
    expect(frame.get_pixel(2, 2, pixel) && pixel != black,
           "reverse winding still fills interior pixels");
    const GsFramebuffer::Vertex d{2.0, 2.0, red};
    const GsFramebuffer::Vertex e{4.0, 4.0, green};
    expect(!frame.draw_triangle(d, e, GsFramebuffer::Vertex{6.0, 6.0, black}),
           "degenerate triangle is rejected");

    frame.clear(black);
    frame.set_scissor(0, 0, 3, 3);
    expect(frame.draw_triangle(a, b, c), "triangle draws under scissor");
    expect(frame.get_pixel(2, 2, pixel) && pixel != black,
           "triangle inside scissor is drawn");
    expect(frame.get_pixel(4, 2, pixel) && pixel == black,
           "triangle outside scissor is clipped");

    frame.reset_scissor();
    frame.clear(red);
    const auto bytes = frame.rgba_bytes();
    expect(bytes.size() == 8U * 8U * 4U, "RGBA export has four bytes per pixel");
    expect(bytes[0] == 255 && bytes[1] == 0 && bytes[2] == 0 && bytes[3] == 255,
           "RGBA export is row-major RGBA");
    expect(bytes[32] == 255 && bytes[35] == 255, "second row follows first row");

    bool rejected = false;
    try { GsFramebuffer invalid(0, 1); }
    catch (const std::invalid_argument&) { rejected = true; }
    expect(rejected, "zero-sized framebuffer is rejected");

    std::cout << "GS framebuffer tests passed\n";
    return 0;
}
