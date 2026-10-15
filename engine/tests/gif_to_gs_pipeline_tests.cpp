#include "retroshelf/gif_to_gs_pipeline.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

namespace {
void append_qword(std::vector<std::uint8_t>& out, std::uint64_t low,
                  std::uint64_t high = 0) {
    for (unsigned i = 0; i < 8; ++i)
        out.push_back(static_cast<std::uint8_t>(low >> (i * 8U)));
    for (unsigned i = 0; i < 8; ++i)
        out.push_back(static_cast<std::uint8_t>(high >> (i * 8U)));
}

std::uint64_t packed_tag(std::uint16_t loops, bool eop, std::uint8_t nreg) {
    return static_cast<std::uint64_t>(loops) |
           (static_cast<std::uint64_t>(eop) << 15U) |
           (static_cast<std::uint64_t>(nreg) << 60U);
}

std::uint64_t xyz(std::uint16_t x, std::uint16_t y) {
    return static_cast<std::uint64_t>(x) |
           (static_cast<std::uint64_t>(y) << 16U);
}
}

int main() {
    int failures = 0;
    const auto check = [&failures](bool ok, const char* name) {
        if (!ok) {
            std::cerr << "FAIL: " << name << '\n';
            ++failures;
        } else {
            std::cout << "PASS: " << name << '\n';
        }
    };

    retroshelf::GifToGsPipeline pipeline(8, 8);
    std::vector<std::uint8_t> dma_payload;

    // GIF PACKED tag 1: PRIM, RGBAQ, and vertex 1.
    append_qword(dma_payload, packed_tag(1, false, 3), 0x510U);
    append_qword(dma_payload, 3U);          // PRIM: triangle list
    append_qword(dma_payload, 0xFF0000FFU); // RGBAQ: opaque red
    append_qword(dma_payload, xyz(16, 16)); // XYZ2: (1,1)

    // GIF PACKED tag 2 completes the triangle and ends the GIF packet.
    append_qword(dma_payload, packed_tag(2, true, 1), 0x5U);
    append_qword(dma_payload, xyz(96, 16)); // XYZ2: (6,1)
    append_qword(dma_payload, xyz(16, 96)); // XYZ2: (1,6)

    check(pipeline.submit_dma_payload(dma_payload),
          "DMA payload is packetized and passed into GS");
    check(pipeline.finish_stream(), "complete GIF stream finishes cleanly");
    check(pipeline.gif_packets_submitted() == 1,
          "one EOP-framed GIF packet reaches GS");
    check(pipeline.gs().packets_processed() == 1,
          "GS command processor sees the packet");
    check(pipeline.gs().triangles_rendered() == 1,
          "GS command processor renders the triangle");

    retroshelf::GsFramebuffer::Pixel pixel{};
    check(pipeline.gs().framebuffer().get_pixel(2, 2, pixel) &&
              pixel == retroshelf::GsFramebuffer::Pixel{255, 0, 0, 255},
          "end-to-end GIF payload produces an opaque red framebuffer pixel");

    pipeline.reset();
    check(pipeline.gif_packets_submitted() == 0 &&
              pipeline.gs().packets_processed() == 0 &&
              pipeline.gs().triangles_rendered() == 0,
          "reset clears packet and GS counters");
    check(pipeline.gs().framebuffer().get_pixel(2, 2, pixel) &&
              pixel == retroshelf::GsFramebuffer::Pixel{0, 0, 0, 0},
          "reset clears framebuffer contents");

    return failures == 0 ? 0 : 1;
}
