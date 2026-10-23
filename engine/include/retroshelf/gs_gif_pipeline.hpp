#pragma once

#include <string>
#include <vector>

#include "retroshelf/gs_packet_processor.hpp"
#include "retroshelf/gs_framebuffer.hpp"
#include "retroshelf/gs_packed_primitive_renderer.hpp"
#include "retroshelf/gs_register_decoder.hpp"
#include "retroshelf/gs_register_state.hpp"
#include "retroshelf/gs_local_memory.hpp"

namespace retroshelf {

// Connects complete PACKED/REGLIST GIF packets to the current GS register model.
// IMAGE and IMAGE2 payloads remain unsupported by the decoder.
class GsGifPipeline {
public:
    bool consume(const std::vector<GifPacket>& packets);
    // Consumes decoded writes and draws supported line, triangle, and sprite primitives.
    // Unsupported GS primitive types remain ignored by the bring-up renderer.
    bool consume_and_render(const std::vector<GifPacket>& packets, GsFramebuffer& target);
    const GsRegisterState& registers() const;
    GsRegisterState& registers();
    const GsLocalMemory& local_memory() const;
    GsLocalMemory& local_memory();
    const std::string& error() const;
    void reset();

private:
    GsRegisterDecoder decoder_;
    GsRegisterState registers_;
    GsLocalMemory local_memory_;
    GsPackedPrimitiveRenderer renderer_;
    std::string error_;
};

} // namespace retroshelf
