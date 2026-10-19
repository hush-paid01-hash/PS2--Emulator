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

namespace retroshelf {

// Connects EE GIF DMA payloads to the incremental GIF parser and GS renderer.
// This is a reusable hardware-path integration layer, not a complete PS2 system.
class EeGifGsRuntime {
public:
    explicit EeGifGsRuntime(std::vector<std::uint8_t>& ee_ram,
                            std::size_t width = 640, std::size_t height = 448);

    // Starts the configured DMA transfer, finalizes the GIF stream, and renders
    // completed packets. On failure, error() explains which stage stopped.
    bool run_frame();
    EeGifDma& dma();
    const GsFramebuffer& framebuffer() const;
    GsFramebuffer& framebuffer();
    const std::string& error() const;

private:
    SystemBus bus_;
    GsPacketProcessor parser_;
    GsGifPipeline pipeline_;
    GsFramebuffer framebuffer_;
    EeGifDma dma_;
    std::string error_;
};

} // namespace retroshelf
