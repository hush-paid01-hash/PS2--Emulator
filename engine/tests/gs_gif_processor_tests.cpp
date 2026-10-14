#include "retroshelf/gs_gif_processor.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

int main() {
    using namespace retroshelf::gs;
    SoftwareRenderer renderer(32, 32);
    const auto background = renderer.pixels().front();
    // PACKED GIFtag: 4 loops, EOP, triangle PRIM / RGBAQ / XYZ2 register sequence.
    const std::uint64_t words[] = {
        4ULL | (1ULL << 15U) | (1ULL << 60U), 0x0000000000000510ULL,
        3ULL, 0ULL,                                      // PRIM = triangle list
        0x000000000000FFFFULL, 0ULL,                     // opaque white RGBAQ
        2ULL | (2ULL << 16U), 0ULL,                     // vertex 1
        28ULL | (2ULL << 16U), 0ULL,                    // vertex 2
        12ULL | (28ULL << 16U), 0ULL                    // vertex 3
    };
    GifProcessStats stats{};
    std::string error;
    if (!process_gif_stream(words, 5, renderer, &stats, &error)) {
        std::cerr << error << '\n'; return 1;
    }
    if (stats.packets != 1 || stats.register_writes != 4 || stats.triangles_drawn != 1) return 2;
    if (std::count_if(renderer.pixels().begin(), renderer.pixels().end(),
                      [background](std::uint32_t p) { return p != background; }) < 20) return 3;

    const std::uint64_t unsupported[] = {
        1ULL | (1ULL << 15U) | (1ULL << 60U), 0x5ULL,
        0ULL, 0ULL
    };
    if (process_gif_stream(unsupported, 2, renderer, nullptr, &error)) return 4;
    if (error.empty()) return 5;
    std::cout << "GIF processor tests passed\n";
    return 0;
}
