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
        // PACKED A+D must address actual GS registers rather than register-list slot E.
        std::vector<std::uint8_t> bytes(32, 0);
        const std::uint64_t tag = 1ULL | (1ULL << 15U) | (1ULL << 60U);
        write_u64(bytes, 0, tag);
        write_u64(bytes, 8, 0xEULL);
        write_u64(bytes, 16, 0xAABBCCDDEEFF0011ULL);
        write_u64(bytes, 24, 0x50ULL); // BITBLTBUF address
        retroshelf::GsGifPipeline pipeline;
        check(pipeline.consume({retroshelf::GifPacket{bytes, 1}}),
              "pipeline consumes PACKED A+D register write");
        std::uint64_t value = 0;
        check(pipeline.registers().read(0x50U, value) &&
                  value == 0xAABBCCDDEEFF0011ULL,
              "A+D value reaches the addressed GS register");
        check(pipeline.local_memory().bytes().size() ==
                  retroshelf::GsLocalMemory::SizeBytes,
              "GIF pipeline owns a 4 MiB GS local-memory store");
        pipeline.local_memory().write_byte(0x1234U, 0xA5U);
        check(pipeline.local_memory().read_byte(0x1234U) == 0xA5U,
              "GS local memory is accessible through the GIF pipeline");
        pipeline.reset();
        check(pipeline.local_memory().read_byte(0x1234U) == 0,
              "pipeline reset clears GS local memory");
    }

    {
        // REGLIST carries two 64-bit register values in each qword.
        std::vector<std::uint8_t> bytes(32, 0);
        const std::uint64_t tag = 1ULL | (1ULL << 15U) | (1ULL << 58U) |
                                  (2ULL << 60U);
        write_u64(bytes, 0, tag);
        write_u64(bytes, 8, 0x21ULL); // Register list {1,2}.
        write_u64(bytes, 16, 0xCAFEULL);
        write_u64(bytes, 24, 0xBABEULL);

        retroshelf::GsGifPipeline pipeline;
        check(pipeline.consume({retroshelf::GifPacket{bytes, 1}}),
              "REGLIST GIF packet is decoded and consumed");
        std::uint64_t first = 0;
        std::uint64_t second = 0;
        check(pipeline.registers().read(1, first) && first == 0xCAFEULL,
              "first REGLIST value reaches the correct GS register");
        check(pipeline.registers().read(2, second) && second == 0xBABEULL,
              "second REGLIST value reaches the correct GS register");
        check(pipeline.registers().write_count() == 2,
              "REGLIST emits two ordered register writes");
    }

    {
        // IMAGE packets are preserved by the decoder but do not become
        // register writes; pixel upload into GS local memory is a later stage.
        std::vector<std::uint8_t> bytes(32, 0);
        const std::uint64_t image_tag = 1ULL | (1ULL << 15U) | (2ULL << 58U);
        write_u64(bytes, 0, image_tag);
        write_u64(bytes, 16, 0x0123456789ABCDEFULL);
        write_u64(bytes, 24, 0xFEDCBA9876543210ULL);
        retroshelf::GsGifPipeline pipeline;
        check(pipeline.consume({retroshelf::GifPacket{bytes, 1}}),
              "pipeline accepts a complete IMAGE payload");
        check(pipeline.registers().write_count() == 0,
              "IMAGE payload does not create bogus GS register writes");
        check(pipeline.image_transfers().size() == 1 &&
                  pipeline.image_transfers()[0].format == 2U &&
                  pipeline.image_transfers()[0].data.size() == 16U,
              "pipeline exposes IMAGE bytes to the GS transfer stage");
        check(pipeline.image_transfers()[0].data[0] == 0xEFU &&
                  pipeline.image_transfers()[0].data[15] == 0xFEU,
              "pipeline preserves the original IMAGE payload byte order");
        check(pipeline.local_memory().read_byte(0) == 0,
              "raw IMAGE bytes are not incorrectly written as linear VRAM");
    }


    {
        std::vector<std::uint8_t> setup(16U + 4U * 16U, 0);
        write_u64(setup, 0, 4ULL | (1ULL << 15U) | (1ULL << 60U));
        write_u64(setup, 8, 0xEULL);
        write_u64(setup, 16, (1ULL << 48U)); write_u64(setup, 24, 0x50ULL);
        write_u64(setup, 32, 0ULL); write_u64(setup, 40, 0x51ULL);
        write_u64(setup, 48, 1ULL | (1ULL << 32U)); write_u64(setup, 56, 0x52ULL);
        write_u64(setup, 64, 0ULL); write_u64(setup, 72, 0x53ULL);
        std::vector<std::uint8_t> image(32U, 0);
        write_u64(image, 0, 1ULL | (1ULL << 15U) | (2ULL << 58U));
        image[16] = 0x11; image[17] = 0x22; image[18] = 0x33; image[19] = 0x44;
        retroshelf::GsGifPipeline pipeline;
        check(pipeline.consume({retroshelf::GifPacket{setup, 1}, retroshelf::GifPacket{image, 1}}),
              "pipeline decodes transfer setup and IMAGE payload");
        check(pipeline.local_memory().read_byte(0) == 0x11 &&
                  pipeline.local_memory().read_byte(1) == 0x22 &&
                  pipeline.local_memory().read_byte(2) == 0x33 &&
                  pipeline.local_memory().read_byte(3) == 0x44,
              "configured PSMCT32 IMAGE upload is applied automatically during consume");
        std::string upload_error;
        check(pipeline.upload_image(upload_error), "explicit PSMCT32 IMAGE upload remains available");
        check(pipeline.local_memory().read_byte(0) == 0x11 &&
                  pipeline.local_memory().read_byte(1) == 0x22 &&
                  pipeline.local_memory().read_byte(2) == 0x33 &&
                  pipeline.local_memory().read_byte(3) == 0x44,
              "PSMCT32 origin pixel writes the expected four bytes");
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
        check(pipeline.image_transfers().empty(),
              "pipeline reset clears queued IMAGE transfer payloads");
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

    {
        // Verify sprites reach the renderer through real packed GIF decoding.
        std::vector<std::uint8_t> bytes(4U * 32U, 0);
        const std::uint8_t addresses[] = {0x0, 0x1, 0x5, 0x5};
        const std::uint64_t values[] = {
            0x6ULL, 0xFF00FF00ULL,
            (static_cast<std::uint64_t>(2U << 4U) << 16U) | (2U << 4U),
            (static_cast<std::uint64_t>(5U << 4U) << 16U) | (5U << 4U)
        };
        for (std::size_t i = 0; i < 4U; ++i) {
            const std::uint64_t tag = 1ULL | ((i == 3U) ? (1ULL << 15U) : 0ULL) |
                                      (1ULL << 60U);
            const std::size_t offset = i * 32U;
            write_u64(bytes, offset, tag);
            write_u64(bytes, offset + 8U, addresses[i]);
            write_u64(bytes, offset + 16U, values[i]);
        }
        retroshelf::GsGifPipeline pipeline;
        retroshelf::GsFramebuffer framebuffer;
        check(framebuffer.resize(8, 8), "framebuffer is allocated for sprite pipeline test");
        check(pipeline.consume_and_render({retroshelf::GifPacket{bytes, 1}}, framebuffer),
              "GIF pipeline renders a PACKED sprite");
        retroshelf::GsFramebuffer::Rgba8 pixel{};
        check(framebuffer.get_pixel(3, 3, pixel) && pixel.r == 0 &&
                  pixel.g == 255 && pixel.b == 0 && pixel.a == 255,
              "pipeline output contains the expected opaque green sprite pixel");
    }

    {
        // Verify line-list endpoints pass through GIF decoding and GS writes.
        std::vector<std::uint8_t> bytes(4U * 32U, 0);
        const std::uint8_t addresses[] = {0x0, 0x1, 0x5, 0x5};
        const std::uint64_t values[] = {
            0x1ULL, 0xFFFF0000ULL,
            (static_cast<std::uint64_t>(1U << 4U) << 16U) | (1U << 4U),
            (static_cast<std::uint64_t>(5U << 4U) << 16U) | (5U << 4U)
        };
        for (std::size_t i = 0; i < 4U; ++i) {
            const std::uint64_t tag = 1ULL | ((i == 3U) ? (1ULL << 15U) : 0ULL) |
                                      (1ULL << 60U);
            const std::size_t offset = i * 32U;
            write_u64(bytes, offset, tag);
            write_u64(bytes, offset + 8U, addresses[i]);
            write_u64(bytes, offset + 16U, values[i]);
        }
        retroshelf::GsGifPipeline pipeline;
        retroshelf::GsFramebuffer framebuffer;
        check(framebuffer.resize(8, 8), "framebuffer is allocated for line pipeline test");
        check(pipeline.consume_and_render({retroshelf::GifPacket{bytes, 1}}, framebuffer),
              "GIF pipeline renders a PACKED line list");
        retroshelf::GsFramebuffer::Rgba8 pixel{};
        check(framebuffer.get_pixel(3, 3, pixel) && pixel.r == 0 &&
                  pixel.g == 0 && pixel.b == 255 && pixel.a == 255,
              "pipeline output contains the expected opaque blue line pixel");
    }


    {
        // Configure a 1x1 PSMCT32 upload, then stage its qword-padded IMAGE data.
        const auto make_ad = [](std::uint8_t address, std::uint64_t value) {
            std::vector<std::uint8_t> bytes(32, 0);
            const std::uint64_t tag = 1ULL | (1ULL << 15U) | (1ULL << 60U);
            write_u64(bytes, 0, tag);
            write_u64(bytes, 8, 0xEULL);
            write_u64(bytes, 16, value);
            write_u64(bytes, 24, address);
            return retroshelf::GifPacket{bytes, 1};
        };
        std::vector<std::uint8_t> image_bytes(32, 0);
        write_u64(image_bytes, 0, 1ULL | (1ULL << 15U) | (2ULL << 58U));
        image_bytes[16] = 0x11; image_bytes[17] = 0x22;
        image_bytes[18] = 0x33; image_bytes[19] = 0x44;

        retroshelf::GsGifPipeline pipeline;
        const std::vector<retroshelf::GifPacket> packets{
            make_ad(0x50, 0), // destination PSMCT32
            make_ad(0x52, 1ULL | (1ULL << 32U)), // 1x1
            make_ad(0x53, 0), // host-to-local
            retroshelf::GifPacket{image_bytes, 1}
        };
        check(pipeline.consume(packets), "pipeline accepts configured IMAGE upload");
        std::vector<std::uint8_t> packed{0xFE};
        std::string error;
        check(pipeline.stage_image_upload(packed, error), "pipeline stages upload against live GS registers");
        check(packed == std::vector<std::uint8_t>({0x11, 0x22, 0x33, 0x44}),
              "pipeline staging strips only final GIF qword padding");
        check(pipeline.local_memory().read_byte(0) == 0x11 &&
                  pipeline.local_memory().read_byte(1) == 0x22 &&
                  pipeline.local_memory().read_byte(2) == 0x33 &&
                  pipeline.local_memory().read_byte(3) == 0x44,
              "automatic PSMCT32 upload updates GS memory while staging remains inspectable");
    }


    {
        // TRXPOS reverse flags control the destination traversal direction;
        // source IMAGE bytes remain in transfer order.
        const auto make_ad = [](std::uint8_t address, std::uint64_t value) {
            std::vector<std::uint8_t> bytes(32, 0);
            write_u64(bytes, 0, 1ULL | (1ULL << 15U) | (1ULL << 60U));
            write_u64(bytes, 8, 0xEULL);
            write_u64(bytes, 16, value);
            write_u64(bytes, 24, address);
            return retroshelf::GifPacket{bytes, 1};
        };
        std::vector<std::uint8_t> image(32, 0);
        write_u64(image, 0, 1ULL | (1ULL << 15U) | (2ULL << 58U));
        for (std::size_t pixel = 0; pixel < 4; ++pixel) {
            image[16U + pixel * 4U] = static_cast<std::uint8_t>(0x10U + pixel);
            image[17U + pixel * 4U] = static_cast<std::uint8_t>(0x20U + pixel);
            image[18U + pixel * 4U] = static_cast<std::uint8_t>(0x30U + pixel);
            image[19U + pixel * 4U] = static_cast<std::uint8_t>(0x40U + pixel);
        }
        retroshelf::GsGifPipeline pipeline;
        const std::vector<retroshelf::GifPacket> packets{
            make_ad(0x50, 0ULL),
            make_ad(0x51, (1ULL << 59U) | (1ULL << 60U)), // reverse X and Y
            make_ad(0x52, 2ULL | (2ULL << 32U)), // 2x2 transfer
            make_ad(0x53, 0ULL),
            retroshelf::GifPacket{image, 1}
        };
        check(pipeline.consume(packets), "pipeline consumes reversed 2x2 PSMCT32 transfer");
        const auto& memory = pipeline.local_memory();
        check(memory.read_byte(0) == 0x13U && memory.read_byte(4) == 0x12U &&
                  memory.read_byte(8) == 0x11U && memory.read_byte(12) == 0x10U,
              "TRXPOS reverse X/Y map source pixels to reversed GS destination coordinates");
    }

    return failures == 0 ? 0 : 1;
}
