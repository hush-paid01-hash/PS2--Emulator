#pragma once

#include "retroshelf/gs_command_processor.hpp"
#include "retroshelf/gs_packet_processor.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf {

// Joins DMA-delivered GIF bytes to EOP-framed packets and the GS renderer.
// A caller can pass this object's submit callback to EeGifDma.
class GifToGsPipeline {
public:
    explicit GifToGsPipeline(std::size_t width = 640, std::size_t height = 448);

    bool submit_dma_payload(const std::vector<std::uint8_t>& bytes);
    bool finish_stream();
    const std::string& error() const;
    GsCommandProcessor& gs();
    const GsCommandProcessor& gs() const;
    std::uint64_t gif_packets_submitted() const;
    void reset();

private:
    GsPacketProcessor packetizer_;
    GsCommandProcessor gs_;
    std::string error_;
    std::uint64_t gif_packets_submitted_{0};
};

} // namespace retroshelf
