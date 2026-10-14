#include "retroshelf/gs_renderer.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
int main() {
    retroshelf::gs::SoftwareRenderer renderer(32, 24);
    if (renderer.width() != 32 || renderer.height() != 24) return 1;
    const auto background = renderer.pixels().front();
    if (!renderer.draw_triangle({2, 2, 0xFF0000FFU},
                                {28, 3, 0x00FF00FFU},
                                {12, 21, 0x0000FFFFU})) return 2;
    const auto changed = std::count_if(renderer.pixels().begin(), renderer.pixels().end(),
        [background](std::uint32_t p) { return p != background; });
    if (changed < 20) { std::cerr << "triangle drew too few pixels: " << changed << '\n'; return 3; }
    if (renderer.draw_triangle({1,1,0},{2,2,0},{3,3,0})) return 4;
    renderer.clear(0x112233FFU);
    if (renderer.pixels().front() != 0x112233FFU) return 5;
    retroshelf::gs::SoftwareRenderer invalid(0, 12);
    if (invalid.draw_triangle({0,0,0},{1,0,0},{0,1,0})) return 6;
    std::cout << "Software renderer tests passed; pixels drawn: " << changed << '\n';
    return 0;
}
