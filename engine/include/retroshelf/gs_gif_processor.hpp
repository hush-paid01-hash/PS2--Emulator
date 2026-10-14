#pragma once

#include "retroshelf/gs_renderer.hpp"
#include "retroshelf/gs_gif_stream.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace retroshelf::gs {

struct GifProcessStats {
    std::size_t packets = 0;
    std::size_t register_writes = 0;
    std::size_t triangles_drawn = 0;
};

// First executable GIF path: supports PACKED PRIM, RGBAQ and XYZ2 for
// untextured triangle-list primitives. Other GS commands fail explicitly.
bool process_gif_stream(const std::uint64_t* words, std::size_t qword_count,
                        SoftwareRenderer& renderer, GifProcessStats* stats = nullptr,
                        std::string* error = nullptr);

} // namespace retroshelf::gs
