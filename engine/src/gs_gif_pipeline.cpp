#include "retroshelf/gs_gif_pipeline.hpp"

namespace retroshelf {

bool GsGifPipeline::consume(const std::vector<GifPacket>& packets) {
    error_.clear();
    for (const auto& packet : packets) {
        if (!decoder_.decode_packed(packet.bytes)) {
            error_ = decoder_.error();
            return false;
        }
        for (const auto& write : decoder_.writes()) {
            if (!registers_.write(write.address, write.value)) {
                error_ = "GS register address is outside the register file";
                return false;
            }
        }
    }
    return true;
}

bool GsGifPipeline::consume_and_render(const std::vector<GifPacket>& packets, GsFramebuffer& target) {
    error_.clear();
    for (const auto& packet : packets) {
        if (!decoder_.decode_packed(packet.bytes)) {
            error_ = decoder_.error();
            return false;
        }
        for (const auto& write : decoder_.writes()) {
            if (!registers_.write(write.address, write.value)) {
                error_ = "GS register address is outside the register file";
                return false;
            }
        }
        if (!renderer_.consume(decoder_.writes(), target)) {
            error_ = renderer_.error();
            return false;
        }
    }
    return true;
}

const GsRegisterState& GsGifPipeline::registers() const { return registers_; }
GsRegisterState& GsGifPipeline::registers() { return registers_; }
const std::string& GsGifPipeline::error() const { return error_; }

void GsGifPipeline::reset() {
    decoder_.clear();
    registers_.reset();
    renderer_.reset();
    error_.clear();
}

} // namespace retroshelf
