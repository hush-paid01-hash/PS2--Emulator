#include "retroshelf/gs_gif_pipeline.hpp"

namespace retroshelf {

bool GsGifPipeline::consume(const std::vector<GifPacket>& packets) {
    error_.clear();
    image_transfers_.clear();
    for (const auto& packet : packets) {
        if (!decoder_.decode_gif(packet.bytes)) {
            error_ = decoder_.error();
            return false;
        }
        image_transfers_.insert(image_transfers_.end(),
                                decoder_.image_transfers().begin(),
                                decoder_.image_transfers().end());
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
    image_transfers_.clear();
    for (const auto& packet : packets) {
        if (!decoder_.decode_gif(packet.bytes)) {
            error_ = decoder_.error();
            return false;
        }
        image_transfers_.insert(image_transfers_.end(),
                                decoder_.image_transfers().begin(),
                                decoder_.image_transfers().end());
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
const std::vector<GsRegisterDecoder::ImageTransfer>& GsGifPipeline::image_transfers() const {
    return image_transfers_;
}
const GsLocalMemory& GsGifPipeline::local_memory() const { return local_memory_; }
GsLocalMemory& GsGifPipeline::local_memory() { return local_memory_; }
const std::string& GsGifPipeline::error() const { return error_; }

void GsGifPipeline::reset() {
    decoder_.clear();
    registers_.reset();
    local_memory_.reset();
    image_transfers_.clear();
    renderer_.reset();
    error_.clear();
}

} // namespace retroshelf
