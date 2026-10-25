#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "retroshelf/gs_framebuffer.hpp"
#include "retroshelf/gs_register_decoder.hpp"
#include "retroshelf/gs_software_rasterizer.hpp"

namespace retroshelf {

// Minimal GS PACKED-write consumer for bring-up. Supports flat-colour triangle
// lists, strips, fans, and sprites (PRIM, RGBAQ, XYZ2); other types are ignored.
class GsPackedPrimitiveRenderer {
public:
    bool consume(const std::vector<GsRegisterDecoder::Write>& writes,
                 GsFramebuffer& target);
    const std::string& error() const;
    void reset();

private:
    GsSoftwareRasterizer rasterizer_;
    std::uint64_t prim_{0};
    GsFramebuffer::Rgba8 color_{};
    std::vector<GsSoftwareRasterizer::Point> line_vertices_;
    std::vector<GsSoftwareRasterizer::Point> triangle_vertices_;
    std::vector<GsSoftwareRasterizer::Point> sprite_vertices_;
    std::string error_;
};

} // namespace retroshelf
