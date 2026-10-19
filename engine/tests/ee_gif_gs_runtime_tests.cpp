#include "retroshelf/ee_gif_gs_runtime.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

namespace {
void write_u64(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint64_t value) {
    for (std::size_t i = 0; i < 8; ++i)
        bytes[offset + i] = static_cast<std::uint8_t>(value >> (i * 8U));
}
}

int main() {
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) {
            std::cerr << "FAIL: " << label << "\n";
            ++failures;
        } else {
            std::cout << "PASS: " << label << "\n";
        }
    };

    // A normal-mode EE GIF DMA transfer feeds five PACKED writes into the
    // parser and renderer without manually wiring the individual components.
    std::vector<std::uint8_t> ram(256, 0);
    const std::uint8_t addresses[] = {0x0, 0x1, 0x5, 0x5, 0x5};
    const std::uint64_t values[] = {
        0x3ULL, 0xFF0000FFULL,
        (static_cast<std::uint64_t>(1U << 4U) << 16U) | (1U << 4U),
        (static_cast<std::uint64_t>(1U << 4U) << 16U) | (6U << 4U),
        (static_cast<std::uint64_t>(6U << 4U) << 16U) | (1U << 4U)
    };
    for (std::size_t i = 0; i < 5; ++i) {
        const std::size_t offset = i * 32U;
        const std::uint64_t tag = 1ULL | (i == 4 ? (1ULL << 15U) : 0ULL) |
                                  (1ULL << 60U);
        write_u64(ram, offset, tag);
        write_u64(ram, offset + 8U, addresses[i]);
        write_u64(ram, offset + 16U, values[i]);
        write_u64(ram, offset + 24U, 0);
    }

    retroshelf::EeGifGsRuntime runtime(ram, 8, 8);
    runtime.dma().registers.chcr = 1U << 8U;
    runtime.dma().registers.madr = 0;
    runtime.dma().registers.qwc = 10;
    check(runtime.run_frame(), runtime.error().empty()
                                   ? "EE GIF DMA runs through parser and GS renderer"
                                   : runtime.error().c_str());
    check(runtime.dma().state() == retroshelf::EeGifDma::State::Completed &&
              runtime.dma().registers.qwc == 0,
          "runtime reports completed DMA transfer");
    retroshelf::GsFramebuffer::Rgba8 pixel{};
    check(runtime.framebuffer().get_pixel(2, 2, pixel) &&
              pixel.r == 255 && pixel.g == 0 && pixel.b == 0 && pixel.a == 255,
          "runtime framebuffer contains the rendered triangle pixel");

    // An invalid transfer should surface a useful error and never claim a frame.
    runtime.dma().registers.chcr = 1U << 8U;
    runtime.dma().registers.madr = 255;
    runtime.dma().registers.qwc = 1;
    check(!runtime.run_frame() && !runtime.error().empty(),
          "runtime propagates DMA failures to its caller");
    return failures == 0 ? 0 : 1;
}
