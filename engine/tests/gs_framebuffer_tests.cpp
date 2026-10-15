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
    GsFramebuffer frame(3, 2);

    expect(frame.width() == 3 && frame.height() == 2, "dimensions are retained");
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
    expect(!frame.get_pixel(3, 0, pixel), "out-of-range read is rejected");

    frame.reset_scissor();
    frame.clear(red);
    const auto bytes = frame.rgba_bytes();
    expect(bytes.size() == 3U * 2U * 4U, "RGBA export has four bytes per pixel");
    expect(bytes[0] == 255 && bytes[1] == 0 && bytes[2] == 0 && bytes[3] == 255,
           "RGBA export is row-major RGBA");
    expect(bytes[12] == 255 && bytes[15] == 255, "second row follows first row");

    bool rejected = false;
    try { GsFramebuffer invalid(0, 1); }
    catch (const std::invalid_argument&) { rejected = true; }
    expect(rejected, "zero-sized framebuffer is rejected");

    std::cout << "GS framebuffer tests passed\n";
    return 0;
}
