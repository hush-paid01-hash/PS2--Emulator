#include "retroshelf/gs_gif_pipeline.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

namespace {
void write_u64(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint64_t value) {
    for (std::size_t i = 0; i < 8; ++i) {
        bytes[offset + i] = static_cast<std::uint8_t>(value >> (i * 8U));
    }
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

    {
        std::vector<std::uint8_t> bytes(32, 0);
        const std::uint64_t tag = 1ULL | (1ULL << 15U) | (1ULL << 60U);
        write_u64(bytes, 0, tag);
        write_u64(bytes, 8, 0x2ULL); // One PACKED register slot: GS register 2.
        write_u64(bytes, 16, 0x1122334455667788ULL);
        write_u64(bytes, 24, 0xAABBCCDDEEFF0011ULL);

        retroshelf::GsGifPipeline pipeline;
        check(pipeline.consume({retroshelf::GifPacket{bytes, 1}}),
              "PACKED GIF packet is decoded and consumed");
        std::uint64_t value = 0;
        check(pipeline.registers().read(2, value) && value == 0x1122334455667788ULL,
              "PACKED register write updates GS register state");
        check(pipeline.registers().write_count() == 1,
              "one decoded register write increments the GS write counter");
    }

    {
        std::vector<std::uint8_t> bytes(16, 0);
        const std::uint64_t image_tag = 1ULL | (1ULL << 15U) | (2ULL << 58U);
        write_u64(bytes, 0, image_tag);
        retroshelf::GsGifPipeline pipeline;
        check(!pipeline.consume({retroshelf::GifPacket{bytes, 1}}) &&
                  pipeline.error().find("PACKED GIF tags only") != std::string::npos,
              "unsupported IMAGE tags fail clearly instead of pretending to render");
    }

    {
        retroshelf::GsGifPipeline pipeline;
        std::vector<std::uint8_t> bytes(32, 0);
        const std::uint64_t tag = 1ULL | (1ULL << 15U) | (1ULL << 60U);
        write_u64(bytes, 0, tag);
        write_u64(bytes, 8, 0x1ULL);
        write_u64(bytes, 16, 0x55ULL);
        check(pipeline.consume({retroshelf::GifPacket{bytes, 1}}),
              "pipeline accepts a valid packet before reset");
        pipeline.reset();
        std::uint64_t value = 99;
        check(pipeline.registers().read(1, value) && value == 0 &&
                  pipeline.registers().write_count() == 0 && pipeline.error().empty(),
              "reset clears register state and pipeline error");
    }


    {
        // Five one-register PACKED tags exercise the complete path from GIF
        // decoding through GS register writes into the software framebuffer.
        std::vector<std::uint8_t> bytes(5U * 32U, 0);
        const std::uint8_t addresses[] = {0x0, 0x1, 0x5, 0x5, 0x5};
        const std::uint64_t values[] = {
            0x3ULL, 0xFF0000FFULL,
            (static_cast<std::uint64_t>(1U << 4U) << 16U) | (1U << 4U),
            (static_cast<std::uint64_t>(1U << 4U) << 16U) | (6U << 4U),
            (static_cast<std::uint64_t>(6U << 4U) << 16U) | (1U << 4U)
        };
        for (std::size_t i = 0; i < 5U; ++i) {
            const bool last = i == 4U;
            const std::uint64_t tag = 1ULL | (last ? (1ULL << 15U) : 0ULL) |
                                      (1ULL << 60U);
            const std::size_t offset = i * 32U;
            write_u64(bytes, offset, tag);
            write_u64(bytes, offset + 8U, addresses[i]);
            write_u64(bytes, offset + 16U, values[i]);
            write_u64(bytes, offset + 24U, 0ULL);
        }
        retroshelf::GsGifPipeline pipeline;
        retroshelf::GsFramebuffer framebuffer;
        check(framebuffer.resize(8, 8), "framebuffer is allocated for pipeline rendering");
        check(pipeline.consume_and_render({retroshelf::GifPacket{bytes, 1}}, framebuffer),
              "GIF pipeline renders a PACKED triangle into the framebuffer");
        retroshelf::GsFramebuffer::Rgba8 pixel{};
        check(framebuffer.get_pixel(2, 2, pixel) && pixel.r == 255 &&
                  pixel.g == 0 && pixel.b == 0 && pixel.a == 255,
              "pipeline output contains the expected opaque red triangle pixel");
    }

    return failures == 0 ? 0 : 1;
}
