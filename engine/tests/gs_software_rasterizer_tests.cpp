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
    const retroshelf::GsFramebuffer::Rgba8 blue{0, 0, 255, 255};
    check(!rasterizer.draw_triangle(fb, {0, 0}, {4, 0}, {0, 4}, red),
          "empty target is rejected");
    check(!rasterizer.draw_line_gouraud(fb, {0, 0}, {4, 0}, red, blue),
          "Gouraud line rejects an empty target");
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

    fb.clear(black);
    check(rasterizer.draw_line_gouraud(fb, {1, 3}, {5, 3}, red, blue),
          "Gouraud line rasterizes");
    check(fb.get_pixel(1, 3, pixel) && pixel == red,
          "Gouraud line preserves the first endpoint colour");
    check(fb.get_pixel(5, 3, pixel) && pixel == blue,
          "Gouraud line preserves the second endpoint colour");
    check(fb.get_pixel(3, 3, pixel) && pixel.r > 0 && pixel.b > 0 &&
              pixel.g == 0 && pixel.a == 255,
          "Gouraud line interpolates colour channels at its midpoint");
    check(fb.get_pixel(3, 4, pixel) && pixel == black,
          "Gouraud line leaves pixels off its path unchanged");

    fb.clear(black);
    check(rasterizer.draw_line_gouraud(fb, {5, 2}, {1, 2}, blue, red),
          "reversed Gouraud line rasterizes");
    check(fb.get_pixel(5, 2, pixel) && pixel == blue,
          "reversed line keeps its first endpoint colour");
    check(fb.get_pixel(1, 2, pixel) && pixel == red,
          "reversed line keeps its second endpoint colour");

    fb.clear(black);
    check(rasterizer.draw_line_gouraud(fb, {4, 4}, {4, 4}, green, blue),
          "zero-length Gouraud line is accepted");
    check(fb.get_pixel(4, 4, pixel) && pixel == green,
          "zero-length Gouraud line uses its single endpoint colour");

    return failures == 0 ? 0 : 1;
}
