#pragma once

#include "retroshelf/gs_framebuffer.hpp"
#include "retroshelf/gs_register_decoder.hpp"
#include "retroshelf/gs_register_state.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf {

// Decodes PACKED GIF writes, stores GS register state, and rasterizes the
// supported GS triangle-list subset into a host framebuffer.
class GsCommandProcessor {
public:
    explicit GsCommandProcessor(std::size_t width = 640, std::size_t height = 448);
    bool submit_packed_packet(const std::vector<std::uint8_t>& packet);
    const GsRegisterState& state() const;
    GsRegisterState& state();
    const GsFramebuffer& framebuffer() const;
    GsFramebuffer& framebuffer();
    const std::string& error() const;
    std::uint64_t packets_processed() const;
    std::uint64_t writes_processed() const;
    std::uint64_t triangles_rendered() const;
    void reset();

private:
    void consume_render_register(std::uint8_t address, std::uint64_t value);
    GsRegisterDecoder decoder_;
    GsRegisterState state_;
    GsFramebuffer framebuffer_;
    std::string error_;
    std::uint64_t packets_processed_{0};
    std::uint64_t writes_processed_{0};
    std::uint64_t triangles_rendered_{0};
    std::uint64_t prim_{0};
    GsFramebuffer::Pixel current_color_{255, 255, 255, 255};
    std::array<GsFramebuffer::Vertex, 3> triangle_vertices_{};
    std::size_t triangle_vertex_count_{0};
};

} // namespace retroshelf
