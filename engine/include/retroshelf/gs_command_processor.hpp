#pragma once

#include "retroshelf/gs_gif_stream.hpp"
#include "retroshelf/gs_renderer.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf::gs {

// Early GS command path: executes GIF PACKED A+D writes for PRIM, RGBAQ,
// XYOFFSET and XYZ2, and rasterizes untextured triangle-list primitives.
// It is intentionally not a complete or cycle-accurate PS2 GS.
class GsCommandProcessor {
public:
    explicit GsCommandProcessor(std::uint32_t width, std::uint32_t height);
    void reset();
    bool execute_gif(const std::uint64_t* words, std::size_t qword_count,
                     std::string* error = nullptr);
    const SoftwareRenderer& renderer() const { return renderer_; }
    std::uint64_t register_value(std::uint8_t address) const;
    std::size_t triangles_drawn() const { return triangles_drawn_; }

private:
    bool write_register(std::uint8_t address, std::uint64_t value);
    void submit_vertex(std::uint64_t value);
    SoftwareRenderer renderer_;
    std::uint64_t registers_[128]{};
    ScreenVertex vertices_[3]{};
    std::size_t vertex_count_ = 0;
    std::size_t triangles_drawn_ = 0;
    std::uint32_t rgba_ = 0xFFFFFFFFU;
};

} // namespace retroshelf::gs
