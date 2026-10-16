#pragma once

#include <string>
#include <vector>

#include "retroshelf/gs_packet_processor.hpp"
#include "retroshelf/gs_framebuffer.hpp"
#include "retroshelf/gs_packed_primitive_renderer.hpp"
#include "retroshelf/gs_register_decoder.hpp"
#include "retroshelf/gs_register_state.hpp"

namespace retroshelf {

// Connects complete GIF packets to the current GS PACKED-tag register model.
// REGLIST and IMAGE payloads remain explicitly unsupported by this first pipeline.
class GsGifPipeline {
public:
    bool consume(const std::vector<GifPacket>& packets);
    // Consumes supported PACKED writes and draws supported triangle lists.
    // Unsupported GS primitive types remain ignored by the bring-up renderer.
    bool consume_and_render(const std::vector<GifPacket>& packets, GsFramebuffer& target);
    const GsRegisterState& registers() const;
    GsRegisterState& registers();
    const std::string& error() const;
    void reset();

private:
    GsRegisterDecoder decoder_;
    GsRegisterState registers_;
    GsPackedPrimitiveRenderer renderer_;
    std::string error_;
};

} // namespace retroshelf
