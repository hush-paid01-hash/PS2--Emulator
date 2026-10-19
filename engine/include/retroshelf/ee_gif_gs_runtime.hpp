#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "retroshelf/ee_gif_dma.hpp"
#include "retroshelf/gs_framebuffer.hpp"
#include "retroshelf/gs_gif_pipeline.hpp"
#include "retroshelf/gs_packet_processor.hpp"
#include "retroshelf/system_bus.hpp"
#include "retroshelf/ps2_ee_memory_map.hpp"

namespace retroshelf {

// Connects EE GIF DMA payloads to the incremental GIF parser and GS renderer.
// This is a reusable hardware-path integration layer, not a complete PS2 system.
class EeGifGsRuntime {
public:
    explicit EeGifGsRuntime(std::vector<std::uint8_t>& ee_ram,
                            std::size_t width = 640, std::size_t height = 448);
    ~EeGifGsRuntime();

    // Starts the configured DMA transfer, finalizes the GIF stream, and renders
    // completed packets. On failure, error() explains which stage stopped.
    bool attach_memory_map(Ps2EeMemoryMap& memory);
    bool run_frame();
    EeGifDma& dma();
    const GsFramebuffer& framebuffer() const;
    GsFramebuffer& framebuffer();
    const std::string& error() const;

private:
    bool read_mmio(std::uint32_t address, std::size_t width, std::uint64_t& value, std::string* error);
    bool write_mmio(std::uint32_t address, std::size_t width, std::uint64_t value, std::string* error);
    std::vector<std::uint8_t>& ee_ram_;
    Ps2EeMemoryMap* attached_memory_{nullptr};
    SystemBus bus_;
    GsPacketProcessor parser_;
    GsGifPipeline pipeline_;
    GsFramebuffer framebuffer_;
    EeGifDma dma_;
    std::string error_;
};

} // namespace retroshelf
