#include "retroshelf/gif_to_gs_pipeline.hpp"

namespace retroshelf {

GifToGsPipeline::GifToGsPipeline(std::size_t width, std::size_t height)
    : gs_(width, height) {}

bool GifToGsPipeline::submit_dma_payload(const std::vector<std::uint8_t>& bytes) {
    error_.clear();
    if (!packetizer_.submit(bytes)) {
        error_ = packetizer_.error();
        return false;
    }
    for (const auto& packet : packetizer_.take_packets()) {
        if (!gs_.submit_packed_packet(packet.bytes)) {
            error_ = gs_.error();
            return false;
        }
        ++gif_packets_submitted_;
    }
    return true;
}

bool GifToGsPipeline::finish_stream() {
    error_.clear();
    if (!packetizer_.finish_stream()) {
        error_ = packetizer_.error();
        return false;
    }
    for (const auto& packet : packetizer_.take_packets()) {
        if (!gs_.submit_packed_packet(packet.bytes)) {
            error_ = gs_.error();
            return false;
        }
        ++gif_packets_submitted_;
    }
    return true;
}

const std::string& GifToGsPipeline::error() const { return error_; }
GsCommandProcessor& GifToGsPipeline::gs() { return gs_; }
const GsCommandProcessor& GifToGsPipeline::gs() const { return gs_; }
std::uint64_t GifToGsPipeline::gif_packets_submitted() const { return gif_packets_submitted_; }

void GifToGsPipeline::reset() {
    packetizer_ = GsPacketProcessor{};
    gs_.reset();
    error_.clear();
    gif_packets_submitted_ = 0;
}

} // namespace retroshelf
