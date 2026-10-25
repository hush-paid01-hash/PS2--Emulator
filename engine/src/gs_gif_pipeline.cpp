#include "retroshelf/gs_gif_pipeline.hpp"
#include "retroshelf/gs_image_upload_stager.hpp"

#include <algorithm>
#include <array>

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
            transfer_state_.write_register(write.address, write.value);
            if (!registers_.write(write.address, write.value)) {
                error_ = "GS register address is outside the register file";
                return false;
            }
        }
    }
    return apply_pending_image_upload();
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
            transfer_state_.write_register(write.address, write.value);
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
    return apply_pending_image_upload();
}

bool GsGifPipeline::apply_pending_image_upload() {
    if (image_transfers_.empty()) return true;
    const auto state = transfer_state_.snapshot();
    // Keep generic IMAGE packets and unsupported formats available to callers
    // without treating them as an applied transfer. PSMCT32 uploads are the
    // first pixel format supported end-to-end by this pipeline.
    if (state.width == 0U || state.height == 0U || state.direction != 0U ||
        state.destination_pixel_storage != 0x00U) {
        return true;
    }
    return upload_image(error_);
}

const GsRegisterState& GsGifPipeline::registers() const { return registers_; }
GsRegisterState& GsGifPipeline::registers() { return registers_; }
const std::vector<GsRegisterDecoder::ImageTransfer>& GsGifPipeline::image_transfers() const {
    return image_transfers_;
}
bool GsGifPipeline::stage_image_upload(std::vector<std::uint8_t>& packed_bytes, std::string& error) const {
    GsImageUploadStager stager;
    return stager.stage(transfer_state_, image_transfers_, packed_bytes, error);
}


bool GsGifPipeline::upload_image(std::string& error) {
    std::vector<std::uint8_t> packed;
    if (!stage_image_upload(packed, error)) return false;
    const auto state = transfer_state_.snapshot();
    if (state.destination_pixel_storage != 0x00U) {
        error = "GS local-memory upload currently supports PSMCT32 only";
        return false;
    }
    const std::size_t pixels = static_cast<std::size_t>(state.width) * state.height;
    if (packed.size() != pixels * 4U) {
        error = "PSMCT32 staged payload size does not match transfer dimensions";
        return false;
    }
    static constexpr std::array<std::uint8_t, 32> page_swizzle = {
        0, 1, 4, 5, 16, 17, 20, 21, 2, 3, 6, 7, 18, 19, 22, 23,
        8, 9, 12, 13, 24, 25, 28, 29, 10, 11, 14, 15, 26, 27, 30, 31
    };
    static constexpr std::array<std::uint8_t, 16> block_swizzle = {
        0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15
    };
    const std::size_t pages_per_row = std::max<std::size_t>(state.destination_buffer_width_64, 1U);
    for (std::size_t row = 0; row < state.height; ++row) {
        for (std::size_t col = 0; col < state.width; ++col) {
            const std::size_t x = static_cast<std::size_t>(state.destination_x) + col;
            const std::size_t y = static_cast<std::size_t>(state.destination_y) + row;
            const std::size_t page_x = x / 64U, page_y = y / 32U;
            const std::size_t page = page_y * pages_per_row + page_x;
            const std::size_t block_x = (x % 64U) / 8U, block_y = (y % 32U) / 8U;
            const std::size_t block = page_swizzle[block_y * 8U + block_x];
            const std::size_t local_x = x % 8U, local_y = y % 8U;
            const std::size_t block_byte = static_cast<std::size_t>(
                block_swizzle[(local_y * 8U + local_x) % 16U]) * 4U + (local_y / 2U) * 64U;
            const std::size_t address = static_cast<std::size_t>(state.destination_base_256) * 256U
                                      + page * 8192U + block * 256U + block_byte;
            const std::size_t source = (row * state.width + col) * 4U;
            if (!local_memory_.write(static_cast<std::uint32_t>(address), packed.data() + source, 4U)) {
                error = "PSMCT32 upload exceeded GS local-memory write limits";
                return false;
            }
        }
    }
    error.clear();
    return true;
}

const GsLocalMemory& GsGifPipeline::local_memory() const { return local_memory_; }
GsLocalMemory& GsGifPipeline::local_memory() { return local_memory_; }
const std::string& GsGifPipeline::error() const { return error_; }

void GsGifPipeline::reset() {
    decoder_.clear();
    registers_.reset();
    local_memory_.reset();
    transfer_state_.reset();
    image_transfers_.clear();
    renderer_.reset();
    error_.clear();
}

} // namespace retroshelf
