#include "retroshelf/ee_gif_gs_runtime.hpp"

#include <stdexcept>
#include <utility>

namespace retroshelf {

EeGifGsRuntime::EeGifGsRuntime(std::vector<std::uint8_t>& ee_ram,
                               std::size_t width, std::size_t height)
    : ee_ram_(ee_ram), bus_(ee_ram),
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


bool EeGifGsRuntime::attach_memory_map(Ps2EeMemoryMap& memory) {
    if (&memory.main_ram() != &ee_ram_) {
        error_ = "GIF runtime and EE memory map must share the same RAM vector";
        return false;
    }
    if (attached_memory_ != nullptr && attached_memory_ != &memory) {
        error_ = "GIF runtime is already attached to another EE memory map";
        return false;
    }
    attached_memory_ = &memory;
    memory.set_mmio_handlers(
        [this](std::uint32_t address, std::size_t width, std::uint64_t& value, std::string* error) {
            return read_mmio(address, width, value, error);
        },
        [this](std::uint32_t address, std::size_t width, std::uint64_t value, std::string* error) {
            return write_mmio(address, width, value, error);
        });
    error_.clear();
    return true;
}

bool EeGifGsRuntime::read_mmio(std::uint32_t address, std::size_t width,
                               std::uint64_t& value, std::string* error) {
    if (width != 4) {
        if (error) *error = "GIF DMA registers require 32-bit accesses";
        return false;
    }
    switch (address) {
    case 0x1000A000U: value = dma_.registers.chcr; break;
    case 0x1000A010U: value = dma_.registers.madr; break;
    case 0x1000A020U: value = dma_.registers.qwc; break;
    case 0x1000A030U: value = dma_.registers.tadr; break;
    case 0x1000A040U: value = dma_.registers.asr0; break;
    case 0x1000A050U: value = dma_.registers.asr1; break;
    default:
        if (error) *error = "Unsupported GIF DMA MMIO register";
        return false;
    }
    if (error) error->clear();
    return true;
}

bool EeGifGsRuntime::write_mmio(std::uint32_t address, std::size_t width,
                                std::uint64_t value, std::string* error) {
    if (width != 4) {
        if (error) *error = "GIF DMA registers require 32-bit accesses";
        return false;
    }
    switch (address) {
    case 0x1000A000U:
        dma_.registers.chcr = static_cast<std::uint32_t>(value);
        if ((dma_.registers.chcr & (1U << 8U)) != 0U && !run_frame()) {
            if (error) *error = error_;
            return false;
        }
        break;
    case 0x1000A010U: dma_.registers.madr = static_cast<std::uint32_t>(value); break;
    case 0x1000A020U: dma_.registers.qwc = static_cast<std::uint32_t>(value); break;
    case 0x1000A030U: dma_.registers.tadr = static_cast<std::uint32_t>(value); break;
    case 0x1000A040U: dma_.registers.asr0 = static_cast<std::uint32_t>(value); break;
    case 0x1000A050U: dma_.registers.asr1 = static_cast<std::uint32_t>(value); break;
    default:
        if (error) *error = "Unsupported GIF DMA MMIO register";
        return false;
    }
    if (error) error->clear();
    return true;
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
