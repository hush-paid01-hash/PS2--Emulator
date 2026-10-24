#pragma once

#include <string>
#include <vector>

#include "retroshelf/gs_packet_processor.hpp"
#include "retroshelf/gs_framebuffer.hpp"
#include "retroshelf/gs_packed_primitive_renderer.hpp"
#include "retroshelf/gs_register_decoder.hpp"
#include "retroshelf/gs_register_state.hpp"
#include "retroshelf/gs_local_memory.hpp"
#include "retroshelf/gs_transfer_state.hpp"

namespace retroshelf {

// Connects GIF register writes to the GS model and exposes IMAGE/IMAGE2 payloads.
// Pixel-format-aware transfer into swizzled GS local memory is a separate stage.
class GsGifPipeline {
public:
    bool consume(const std::vector<GifPacket>& packets);
    // Consumes decoded writes and draws supported line, triangle, and sprite primitives.
    // Unsupported GS primitive types remain ignored by the bring-up renderer.
    bool consume_and_render(const std::vector<GifPacket>& packets, GsFramebuffer& target);
    const GsRegisterState& registers() const;
    GsRegisterState& registers();
    // IMAGE/IMAGE2 payloads from the most recent consume call, in packet order.
    const std::vector<GsRegisterDecoder::ImageTransfer>& image_transfers() const;
    // Validates the most recent IMAGE/IMAGE2 payloads against the current transfer registers.
    // Output contains packed bytes only; it is not yet written to swizzled GS local memory.
    bool stage_image_upload(std::vector<std::uint8_t>& packed_bytes, std::string& error) const;
    const GsLocalMemory& local_memory() const;
    GsLocalMemory& local_memory();
    const std::string& error() const;
    void reset();

private:
    GsRegisterDecoder decoder_;
    GsRegisterState registers_;
    GsLocalMemory local_memory_;
    GsTransferState transfer_state_;
    std::vector<GsRegisterDecoder::ImageTransfer> image_transfers_;
    GsPackedPrimitiveRenderer renderer_;
    std::string error_;
};

} // namespace retroshelf
