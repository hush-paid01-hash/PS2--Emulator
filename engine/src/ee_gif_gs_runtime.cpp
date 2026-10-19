#include "retroshelf/ee_gif_gs_runtime.hpp"

#include <stdexcept>
#include <utility>

namespace retroshelf {

EeGifGsRuntime::EeGifGsRuntime(std::vector<std::uint8_t>& ee_ram,
                               std::size_t width, std::size_t height)
    : bus_(ee_ram),
      dma_(bus_,
           [this](const std::vector<std::uint8_t>& payload) {
               if (!parser_.submit(payload)) {
                   throw std::runtime_error(parser_.error());
               }
           }) {
    if (!framebuffer_.resize(width, height)) {
        error_ = "Invalid GS framebuffer dimensions";
    }
}

bool EeGifGsRuntime::run_frame() {
    error_.clear();
    if (framebuffer_.empty()) {
        error_ = "GS framebuffer is not initialized";
        return false;
    }

    parser_ = GsPacketProcessor{};
    pipeline_.reset();
    framebuffer_.clear({0, 0, 0, 255});

    if (!dma_.start()) {
        error_ = dma_.error();
        return false;
    }
    if (!parser_.finish_stream()) {
        error_ = parser_.error();
        return false;
    }

    const auto packets = parser_.take_packets();
    if (packets.empty()) {
        error_ = "GIF DMA completed without a complete GIF packet";
        return false;
    }
    if (!pipeline_.consume_and_render(packets, framebuffer_)) {
        error_ = pipeline_.error();
        return false;
    }
    return true;
}

EeGifDma& EeGifGsRuntime::dma() { return dma_; }
const GsFramebuffer& EeGifGsRuntime::framebuffer() const { return framebuffer_; }
GsFramebuffer& EeGifGsRuntime::framebuffer() { return framebuffer_; }
const std::string& EeGifGsRuntime::error() const { return error_; }

} // namespace retroshelf
