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

    return failures == 0 ? 0 : 1;
}
