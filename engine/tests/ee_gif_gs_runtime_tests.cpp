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

    // Exercise the complete EE RAM -> GIF DMA -> packet parser -> GS
    // transfer-register setup -> IMAGE payload -> swizzled local-memory path.
    std::vector<std::uint8_t> upload_ram(256, 0);
    const std::size_t setup_size = 80U;
    write_u64(upload_ram, 0, 4ULL | (1ULL << 15U) | (1ULL << 60U));
    write_u64(upload_ram, 8, 0xEULL); // PACKED A+D
    write_u64(upload_ram, 16, 0ULL); write_u64(upload_ram, 24, 0x50ULL); // BITBLTBUF: PSMCT32, base 0
    write_u64(upload_ram, 32, 0ULL); write_u64(upload_ram, 40, 0x51ULL); // TRXPOS: origin
    write_u64(upload_ram, 48, 1ULL | (1ULL << 32U)); write_u64(upload_ram, 56, 0x52ULL); // TRXREG 1x1
    write_u64(upload_ram, 64, 0ULL); write_u64(upload_ram, 72, 0x53ULL); // TRXDIR host-to-local
    write_u64(upload_ram, setup_size, 1ULL | (1ULL << 15U) | (2ULL << 58U)); // IMAGE, 1 qword
    upload_ram[setup_size + 16U] = 0x11U;
    upload_ram[setup_size + 17U] = 0x22U;
    upload_ram[setup_size + 18U] = 0x33U;
    upload_ram[setup_size + 19U] = 0x44U;
    retroshelf::EeGifGsRuntime upload_runtime(upload_ram, 8, 8);
    upload_runtime.dma().registers.chcr = 1U << 8U;
    upload_runtime.dma().registers.madr = 0;
    upload_runtime.dma().registers.qwc = 7U; // 5 setup qwords + 2 IMAGE qwords
    check(upload_runtime.run_frame(), upload_runtime.error().empty()
                                       ? "DMA runtime accepts a configured PSMCT32 IMAGE transfer"
                                       : upload_runtime.error().c_str());
    check(upload_runtime.local_memory().read_byte(0) == 0x11U &&
              upload_runtime.local_memory().read_byte(1) == 0x22U &&
              upload_runtime.local_memory().read_byte(2) == 0x33U &&
              upload_runtime.local_memory().read_byte(3) == 0x44U,
          "DMA runtime automatically applies IMAGE pixels to GS local memory");

    // An invalid transfer should surface a useful error and never claim a frame.
    runtime.dma().registers.chcr = 1U << 8U;
    runtime.dma().registers.madr = 255;
    runtime.dma().registers.qwc = 1;
    check(!runtime.run_frame() && !runtime.error().empty(),
          "runtime propagates DMA failures to its caller");
    return failures == 0 ? 0 : 1;
}
