#include "retroshelf/gs_framebuffer.hpp"

#include <iostream>

int main() {
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << "\n"; ++failures; }
        else { std::cout << "PASS: " << label << "\n"; }
    };

    retroshelf::GsFramebuffer fb;
    check(fb.empty() && fb.width() == 0 && fb.height() == 0,
          "new framebuffer starts empty");
    check(!fb.resize(0, 1) && !fb.resize(1, 0),
          "zero dimensions are rejected");
    check(!fb.resize(retroshelf::GsFramebuffer::MaxDimension + 1U, 1),
          "oversized dimensions are rejected");
    check(fb.resize(3, 2), "small framebuffer allocates");
    check(fb.rgba_bytes().size() == 24U,
          "storage contains four bytes per pixel");

    const retroshelf::GsFramebuffer::Rgba8 red{255, 0, 0, 255};
    const retroshelf::GsFramebuffer::Rgba8 blue{0, 0, 255, 255};
    check(fb.set_pixel(2, 1, red), "in-bounds pixel write succeeds");
    retroshelf::GsFramebuffer::Rgba8 pixel{};
    check(fb.get_pixel(2, 1, pixel) && pixel == red,
          "pixel read returns written RGBA value");
    check(!fb.set_pixel(3, 1, red) && !fb.get_pixel(0, 2, pixel),
          "out-of-bounds pixel access is rejected");

    fb.clear(blue);
    check(fb.get_pixel(0, 0, pixel) && pixel == blue &&
              fb.get_pixel(2, 1, pixel) && pixel == blue,
          "clear fills the whole surface");
    check(fb.resize(1, 1) && fb.rgba_bytes().size() == 4U,
          "resize replaces storage and dimensions");

    return failures == 0 ? 0 : 1;
}
