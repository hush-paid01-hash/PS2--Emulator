#include "retroshelf/gs_gif_processor.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>

int main() {
    using namespace retroshelf::gs;
    SoftwareRenderer renderer(32, 32);
    const auto background = renderer.pixels().front();
    // Three PACKED packets: PRIM, RGBAQ, then three XYZ2 vertices.
    const std::uint64_t words[] = {
        1ULL | (1ULL << 60U), 0ULL, // PRIM packet
        3ULL, 0ULL,
        1ULL | (1ULL << 60U), 0x1ULL, // RGBAQ packet
        0x00000000FFFFFFFFULL, 0ULL, // opaque white
        3ULL | (1ULL << 15U) | (1ULL << 60U), 0x5ULL, // EOP XYZ2 packet with three loops
        2ULL | (2ULL << 16U), 0ULL,
        28ULL | (2ULL << 16U), 0ULL,
        12ULL | (28ULL << 16U), 0ULL
    };
    GifProcessStats stats{};
    std::string error;
    if (!process_gif_stream(words, 8, renderer, &stats, &error)) {
        std::cerr << error << '\\n'; return 1;
    }
    if (stats.packets != 3 || stats.register_writes != 5 || stats.triangles_drawn != 1) return 2;
    if (std::count_if(renderer.pixels().begin(), renderer.pixels().end(),
                      [background](std::uint32_t p) { return p != background; }) < 20) return 3;

    const std::uint64_t unsupported[] = {
        1ULL | (1ULL << 15U) | (1ULL << 60U), 0x5ULL,
        0ULL, 0ULL
    };
    if (process_gif_stream(unsupported, 2, renderer, nullptr, &error)) return 4;
    if (error.empty()) return 5;
    std::cout << "GIF processor tests passed\\n";
    return 0;
}
