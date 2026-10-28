#include "retroshelf/gs_software_rasterizer.hpp"

#include <iostream>

int main() {
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << "\n"; ++failures; }
        else { std::cout << "PASS: " << label << "\n"; }
    };

    retroshelf::GsFramebuffer fb;
    retroshelf::GsSoftwareRasterizer rasterizer;
    const retroshelf::GsFramebuffer::Rgba8 black{0, 0, 0, 255};
    const retroshelf::GsFramebuffer::Rgba8 red{255, 0, 0, 255};
    const retroshelf::GsFramebuffer::Rgba8 green{0, 255, 0, 255};
    check(!rasterizer.draw_triangle(fb, {0, 0}, {4, 0}, {0, 4}, red),
          "empty target is rejected");
    check(fb.resize(8, 8), "target allocation succeeds");
    fb.clear(black);
    check(rasterizer.draw_triangle(fb, {1, 1}, {6, 1}, {1, 6}, red),
          "triangle rasterizes");

    retroshelf::GsFramebuffer::Rgba8 pixel{};
    check(fb.get_pixel(2, 2, pixel) && pixel == red,
          "interior triangle pixel is filled");
    check(fb.get_pixel(7, 7, pixel) && pixel == black,
          "pixel outside triangle remains unchanged");

    fb.clear(black);
    check(rasterizer.draw_triangle(fb, {-3, 2}, {4, -2}, {5, 6}, green),
          "partially off-screen triangle clips safely");
    check(fb.get_pixel(0, 2, pixel) && pixel == green,
          "clipped triangle draws within the visible surface");
    check(rasterizer.draw_triangle(fb, {1, 1}, {2, 2}, {3, 3}, red),
          "degenerate triangle is accepted as a no-op");
    check(!rasterizer.draw_triangle(fb, {1000001, 0}, {0, 2}, {2, 0}, red),
          "extreme coordinates are rejected");

    return failures == 0 ? 0 : 1;
}
