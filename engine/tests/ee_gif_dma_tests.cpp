#include "retroshelf/ee_gif_dma.hpp"
#include "retroshelf/gs_packet_processor.hpp"
#include "retroshelf/gs_gif_pipeline.hpp"
#include "retroshelf/gs_framebuffer.hpp"
#include "retroshelf/system_bus.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t Start = 1U << 8U;
constexpr std::uint32_t Direction = 0U;
constexpr std::uint32_t ReverseDirection = 1U;
constexpr std::uint32_t Chain = 1U << 2U;
constexpr std::uint32_t Tie = 1U << 7U;

void write_tag(std::vector<std::uint8_t>& ram, std::uint32_t address, std::uint16_t qwc,
               std::uint8_t id, std::uint32_t target = 0, bool irq = false,
               bool scratchpad = false) {
    const std::uint64_t low = qwc | (static_cast<std::uint64_t>(id) << 28U) |
                              (irq ? (1ULL << 31U) : 0U);
    const std::uint64_t high = target | (scratchpad ? (1ULL << 31U) : 0U);
    for (std::size_t byte = 0; byte < 8; ++byte) {
        ram[address + byte] = static_cast<std::uint8_t>(low >> (byte * 8U));
        ram[address + 8U + byte] = static_cast<std::uint8_t>(high >> (byte * 8U));
    }
}

void write_qword(std::vector<std::uint8_t>& ram, std::uint32_t address,
                 std::uint64_t low, std::uint64_t high = 0) {
    for (std::size_t byte = 0; byte < 8; ++byte) {
        ram[address + byte] = static_cast<std::uint8_t>(low >> (byte * 8U));
        ram[address + 8U + byte] =
            static_cast<std::uint8_t>(high >> (byte * 8U));
    }
}

void fill(std::vector<std::uint8_t>& ram, std::uint32_t address, std::uint8_t value) {
    for (std::size_t byte = 0; byte < retroshelf::EeGifDma::QwordSize; ++byte) {
        ram[address + byte] = value;
    }
}

bool all_bytes(const std::vector<std::uint8_t>& bytes, std::uint8_t value) {
    for (const auto byte : bytes) {
        if (byte != value) return false;
    }
    return true;
}

} // namespace

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
        std::vector<std::uint8_t> ram(512);
        fill(ram, 256, 0xA1);
        write_tag(ram, 0, 1, 0, 256);
        std::vector<std::vector<std::uint8_t>> payloads;
        std::vector<retroshelf::EeGifDma::Interrupt> interrupts;
        retroshelf::EeGifDma dma(ram, [&](const auto& payload) { payloads.push_back(payload); },
                                 [&](auto reason) { interrupts.push_back(reason); });
        dma.registers.chcr = Start | Direction | Chain;
        dma.registers.tadr = 0;
        check(dma.start(), "REFE transfers external payload");
        check(payloads.size() == 1 && all_bytes(payloads[0], 0xA1), "REFE payload contents");
        check(dma.registers.tadr == 16 && dma.registers.madr == 272 &&
                  dma.registers.qwc == 0 && (dma.registers.chcr & Start) == 0,
              "REFE terminal register state");
        check(dma.state() == retroshelf::EeGifDma::State::Completed &&
                  interrupts.size() == 1 &&
                  interrupts[0] == retroshelf::EeGifDma::Interrupt::Completion,
              "REFE completion interrupt");
    }

    {
        std::vector<std::uint8_t> ram(512);
        fill(ram, 16, 0x11);
        write_tag(ram, 0, 1, 1);
        write_tag(ram, 32, 0, 7);
        std::vector<std::vector<std::uint8_t>> payloads;
        retroshelf::EeGifDma dma(ram, [&](const auto& payload) { payloads.push_back(payload); });
        dma.registers.chcr = Start | Direction | Chain;
        dma.registers.tadr = 0;
        check(dma.start() && payloads.size() == 1 && all_bytes(payloads[0], 0x11),
              "CNT transfers inline payload and proceeds after it");
        check(dma.registers.tadr == 48 && dma.registers.madr == 48,
              "CNT and END update TADR and MADR");
    }

    {
        std::vector<std::uint8_t> ram(512);
        fill(ram, 16, 0x21);
        write_tag(ram, 0, 1, 2, 128);
        write_tag(ram, 128, 0, 7);
        std::vector<std::vector<std::uint8_t>> payloads;
        retroshelf::EeGifDma dma(ram, [&](const auto& payload) { payloads.push_back(payload); });
        dma.registers.chcr = Start | Direction | Chain;
        dma.registers.tadr = 0;
        check(dma.start() && payloads.size() == 1 && all_bytes(payloads[0], 0x21),
              "NEXT transfers inline payload then branches");
        check(dma.registers.tadr == 144, "NEXT branches to target tag");
    }

    {
        std::vector<std::uint8_t> ram(512);
        fill(ram, 16, 0x91);
        fill(ram, 256, 0x92);
        fill(ram, 64, 0x93);
        fill(ram, 144, 0x94);
        write_tag(ram, 0, 1, 1);
        write_tag(ram, 32, 1, 3, 256);
        write_tag(ram, 48, 1, 2, 128);
        write_tag(ram, 128, 1, 7);
        std::vector<std::vector<std::uint8_t>> payloads;
        retroshelf::EeGifDma dma(ram, [&](const auto& payload) { payloads.push_back(payload); });
        dma.registers.chcr = Start | Direction | Chain;
        dma.registers.tadr = 0;
        check(dma.start() && payloads.size() == 4 &&
                  all_bytes(payloads[0], 0x91) && all_bytes(payloads[1], 0x92) &&
                  all_bytes(payloads[2], 0x93) && all_bytes(payloads[3], 0x94),
              "mixed chain dispatches payloads in tag order");
    }

    for (const auto id : {3U, 4U}) {
        std::vector<std::uint8_t> ram(512);
        fill(ram, 256, static_cast<std::uint8_t>(0x30U + id));
        write_tag(ram, 0, 1, static_cast<std::uint8_t>(id), 256);
        write_tag(ram, 16, 0, 7);
        std::vector<std::vector<std::uint8_t>> payloads;
        retroshelf::EeGifDma dma(ram, [&](const auto& payload) { payloads.push_back(payload); });
        dma.registers.chcr = Start | Direction | Chain;
        dma.registers.tadr = 0;
        check(dma.start() && payloads.size() == 1 &&
                  all_bytes(payloads[0], static_cast<std::uint8_t>(0x30U + id)),
              id == 3 ? "REF transfers external payload" : "REFS transfers external payload");
        check(dma.registers.tadr == 32, "REF/REFS continue at tag after current tag");
    }

    {
        std::vector<std::uint8_t> ram(512);
        fill(ram, 16, 0x51);
        fill(ram, 144, 0x52);
        write_tag(ram, 0, 0, 5, 128);
        write_tag(ram, 16, 0, 7);
        write_tag(ram, 128, 1, 1);
        write_tag(ram, 160, 0, 6);
        std::vector<std::vector<std::uint8_t>> payloads;
        retroshelf::EeGifDma dma(ram, [&](const auto& payload) { payloads.push_back(payload); });
        dma.registers.chcr = Start | Direction | Chain;
        dma.registers.tadr = 0;
        check(dma.start() && payloads.size() == 1 && all_bytes(payloads[0], 0x52),
              "CALL transfers subroutine payload then RET resumes");
        check(dma.registers.asp == 0 && dma.registers.asr0 == 16 &&
                  (dma.registers.chcr & (3U << 4U)) == 0U && dma.registers.tadr == 32,
              "CALL/RET update stack registers and return address");
    }

    {
        std::vector<std::uint8_t> ram(256);
        write_tag(ram, 0, 0, 5, 64);
        write_tag(ram, 64, 0, 5, 128);
        write_tag(ram, 128, 0, 5, 192);
        retroshelf::EeGifDma dma(ram, [](const auto&) {});
        dma.registers.chcr = Start | Direction | Chain;
        check(!dma.start() && dma.error().find("overflow") != std::string::npos,
              "third nested CALL exceeds the two-entry stack");
    }

    {
        std::vector<std::uint8_t> ram(512);
        fill(ram, 16, 0x61);
        write_tag(ram, 0, 1, 7);
        std::vector<std::vector<std::uint8_t>> payloads;
        retroshelf::EeGifDma dma(ram, [&](const auto& payload) { payloads.push_back(payload); });
        dma.registers.chcr = Start | Direction | Chain;
        dma.registers.tadr = 0;
        check(dma.start() && payloads.size() == 1 && all_bytes(payloads[0], 0x61),
              "END transfers inline payload and terminates");
        check(dma.registers.tadr == 32, "END updates TADR past inline payload");
    }

    {
        std::vector<std::uint8_t> ram(512);
        fill(ram, 16, 0x71);
        write_tag(ram, 0, 1, 1, 0, true);
        std::vector<retroshelf::EeGifDma::Interrupt> interrupts;
        retroshelf::EeGifDma dma(ram, [](const auto&) {},
                                 [&](auto reason) { interrupts.push_back(reason); });
        dma.registers.chcr = Start | Direction | Chain | Tie;
        dma.registers.tadr = 0;
        check(dma.start() && dma.registers.tadr == 32 &&
                  dma.state() == retroshelf::EeGifDma::State::Completed,
              "TIE terminates chain after an IRQ tag");
        check(interrupts.size() == 1 && interrupts[0] == retroshelf::EeGifDma::Interrupt::Tag,
              "tag IRQ signals the interrupt callback");
    }

    {
        std::vector<std::uint8_t> ram(64);
        write_tag(ram, 0, 0, 1, 0, true);
        write_tag(ram, 16, 0, 7);
        std::vector<retroshelf::EeGifDma::Interrupt> interrupts;
        retroshelf::EeGifDma dma(ram, [](const auto&) {},
                                 [&](auto reason) { interrupts.push_back(reason); });
        dma.registers.chcr = Start | Direction | Chain;
        dma.registers.tadr = 0;
        check(dma.start() && dma.registers.tadr == 32 &&
                  dma.state() == retroshelf::EeGifDma::State::Completed,
              "tag IRQ does not stop the chain when TIE is clear");
        check(interrupts.size() == 1 &&
                  interrupts[0] == retroshelf::EeGifDma::Interrupt::Completion,
              "ignored tag IRQ proceeds to the completion interrupt");
    }

    {
        std::vector<std::uint8_t> ram(64);
        std::vector<retroshelf::EeGifDma::Interrupt> interrupts;
        retroshelf::EeGifDma dma(ram, [](const auto&) {},
                                 [&](auto reason) { interrupts.push_back(reason); });
        dma.registers.chcr = Start | Direction | Chain;
        dma.registers.tadr = 64;
        check(!dma.start() && dma.state() == retroshelf::EeGifDma::State::Error &&
                  (dma.registers.chcr & Start) == 0 && !dma.error().empty(),
              "out-of-range tag fails and clears STR");
        check(interrupts.size() == 1 && interrupts[0] == retroshelf::EeGifDma::Interrupt::Error,
              "DMA error signals the interrupt callback");
    }

    {
        std::vector<std::uint8_t> ram(64);
        write_tag(ram, 0, 0, 6);
        retroshelf::EeGifDma dma(ram, [](const auto&) {});
        dma.registers.chcr = Start | Direction | Chain;
        check(!dma.start() && dma.error().find("underflow") != std::string::npos,
              "RET without a matching CALL is malformed");
    }

    {
        std::vector<std::uint8_t> ram(64);
        write_tag(ram, 0, 0, 3, 0, false, true);
        retroshelf::EeGifDma dma(ram, [](const auto&) {});
        dma.registers.chcr = Start | Direction | Chain;
        check(!dma.start() && dma.error().find("scratchpad") != std::string::npos,
              "scratchpad tag address is rejected");
    }

    {
        std::vector<std::uint8_t> ram(64);
        write_tag(ram, 0, 1, 3, 64);
        retroshelf::EeGifDma dma(ram, [](const auto&) {});
        dma.registers.chcr = Start | Direction | Chain;
        check(!dma.start() && dma.error().find("outside EE RAM") != std::string::npos,
              "out-of-range payload is rejected");
    }

    {
        std::vector<std::uint8_t> ram(16);
        write_tag(ram, 0, 0, 2, 0);
        retroshelf::EeGifDma dma(ram, [](const auto&) {});
        dma.registers.chcr = Start | Direction | Chain;
        check(!dma.start() && dma.error().find("tag limit") != std::string::npos,
              "cyclic chain stops at the finite tag limit");
    }

    {
        std::vector<std::uint8_t> ram(64);
        fill(ram, 16, 0x81);
        std::vector<retroshelf::EeGifDma::Interrupt> interrupts;
        retroshelf::EeGifDma dma(ram, [](const auto&) {},
                                 [&](auto reason) { interrupts.push_back(reason); });
        dma.registers.chcr = Start | Direction;
        dma.registers.madr = 16;
        dma.registers.qwc = 1;
        check(dma.start() && dma.registers.madr == 32 && dma.registers.qwc == 0 &&
                  dma.state() == retroshelf::EeGifDma::State::Completed,
              "normal mode remains a direct MADR/QWC transfer");
        check(interrupts.size() == 1 &&
                  interrupts[0] == retroshelf::EeGifDma::Interrupt::Completion,
              "normal EE-RAM-to-GIF transfer signals completion");
    }

    {
        std::vector<std::uint8_t> ram(64);
        fill(ram, 16, 0x82);
        std::vector<retroshelf::EeGifDma::Interrupt> interrupts;
        retroshelf::EeGifDma dma(ram, {},
                                 [&](auto reason) { interrupts.push_back(reason); });
        dma.registers.chcr = Start | Direction;
        dma.registers.madr = 16;
        dma.registers.qwc = 1;
        check(!dma.start() && dma.state() == retroshelf::EeGifDma::State::Error &&
                  (dma.registers.chcr & Start) == 0 && dma.registers.qwc == 0 &&
                  dma.error().find("no payload handler") != std::string::npos,
              "nonempty transfer fails instead of dropping payload without a handler");
        check(interrupts.size() == 1 &&
                  interrupts[0] == retroshelf::EeGifDma::Interrupt::Error,
              "missing payload handler signals a DMA error");
    }

    {
        std::vector<std::uint8_t> ram(64);
        fill(ram, 16, 0x83);
        std::vector<retroshelf::EeGifDma::Interrupt> interrupts;
        retroshelf::EeGifDma dma(ram, [](const auto&) { throw std::runtime_error("GS failed"); },
                                 [&](auto reason) { interrupts.push_back(reason); });
        dma.registers.chcr = Start | Direction;
        dma.registers.madr = 16;
        dma.registers.qwc = 1;
        check(!dma.start() && dma.state() == retroshelf::EeGifDma::State::Error &&
                  (dma.registers.chcr & Start) == 0 && dma.registers.qwc == 0,
              "payload submission failure clears STR and QWC");
        check(interrupts.size() == 1 &&
                  interrupts[0] == retroshelf::EeGifDma::Interrupt::Error,
              "payload submission failure signals a DMA error");
    }

    {
        std::vector<std::uint8_t> ram(128);
        write_tag(ram, 0, 2, 1);
        const std::uint64_t gif_tag = 1U | (1ULL << 15U) | (1ULL << 60U);
        write_qword(ram, 16, gif_tag, 0xE);
        write_qword(ram, 32, 0x1122334455667788ULL, 0x99AABBCCDDEEFF00ULL);
        write_tag(ram, 48, 0, 7);

        retroshelf::SystemBus bus(ram);
        retroshelf::GsPacketProcessor gs;
        std::vector<retroshelf::EeGifDma::Interrupt> interrupts;
        retroshelf::EeGifDma dma(
            bus,
            [&](const auto& payload) {
                if (!gs.submit(payload)) {
                    throw std::runtime_error(gs.error());
                }
            },
            [&](auto reason) { interrupts.push_back(reason); });
        dma.registers.chcr = Start | Direction | Chain;
        dma.registers.tadr = 0;

        check(dma.start() && gs.finish_stream(),
              "chained EE RAM transfer passes through bus, DMA, and GIF parser");
        const auto packets = gs.take_packets();
        std::vector<std::uint8_t> expected(ram.begin() + 16, ram.begin() + 48);
        check(packets.size() == 1 && packets[0].bytes == expected &&
                  packets[0].tag_count == 1,
              "integrated GIF packet preserves source bytes and order");
        check(dma.registers.tadr == 64 && dma.registers.madr == 64 &&
                  dma.registers.qwc == 0 &&
                  dma.state() == retroshelf::EeGifDma::State::Completed &&
                  interrupts.size() == 1 &&
                  interrupts[0] == retroshelf::EeGifDma::Interrupt::Completion,
              "integrated chain updates registers and signals completion once");
    }

    {
        std::vector<std::uint8_t> ram(32);
        write_tag(ram, 0, 2, 1);
        retroshelf::SystemBus bus(ram);
        retroshelf::GsPacketProcessor gs;
        std::vector<retroshelf::EeGifDma::Interrupt> interrupts;
        retroshelf::EeGifDma dma(
            bus,
            [&](const auto& payload) {
                if (!gs.submit(payload)) {
                    throw std::runtime_error(gs.error());
                }
            },
            [&](auto reason) { interrupts.push_back(reason); });
        dma.registers.chcr = Start | Direction | Chain;

        check(!dma.start() && dma.state() == retroshelf::EeGifDma::State::Error &&
                  (dma.registers.chcr & Start) == 0 &&
                  interrupts.size() == 1 &&
                  interrupts[0] == retroshelf::EeGifDma::Interrupt::Error,
              "bus rejects an out-of-range chained payload with one DMA error interrupt");
    }

    {
        std::vector<std::uint8_t> ram(64);
        retroshelf::EeGifDma dma(ram, [](const auto&) {});
        dma.registers.chcr = Start | ReverseDirection;
        check(!dma.start() && dma.state() == retroshelf::EeGifDma::State::Error &&
                  dma.error().find("direction") != std::string::npos,
              "reverse GIF DMA direction is rejected");
    }

    {
        std::vector<std::uint8_t> ram(64);
        retroshelf::SystemBus bus(ram);
        retroshelf::EeGifDma dma(bus, [](const auto&) {});
        dma.registers.chcr = Start | Direction;
        dma.registers.madr = 17;
        dma.registers.qwc = 1;
        check(!dma.start() && dma.state() == retroshelf::EeGifDma::State::Error &&
                  dma.error().find("unaligned") != std::string::npos,
              "system-bus DMA rejects a payload address not aligned to a qword");
    }


    {
        // End-to-end regression: chained EE GIF DMA carries a real PACKED
        // triangle through the persistent GIF parser and software GS renderer.
        std::vector<std::uint8_t> ram(192);
        const std::uint8_t addresses[] = {0x0, 0x1, 0x5, 0x5, 0x5};
        const std::uint64_t values[] = {
            0x3ULL, 0xFF0000FFULL,
            (static_cast<std::uint64_t>(1U << 4U) << 16U) | (1U << 4U),
            (static_cast<std::uint64_t>(1U << 4U) << 16U) | (6U << 4U),
            (static_cast<std::uint64_t>(6U << 4U) << 16U) | (1U << 4U)
        };
        for (std::size_t i = 0; i < 5U; ++i) {
            const std::size_t offset = 16U + i * 32U;
            const std::uint64_t gif_tag = 1ULL | (i == 4U ? (1ULL << 15U) : 0ULL) |
                                          (1ULL << 60U);
            write_qword(ram, static_cast<std::uint32_t>(offset), gif_tag, addresses[i]);
            write_qword(ram, static_cast<std::uint32_t>(offset + 16U), values[i], 0ULL);
        }
        write_tag(ram, 0, 10, 1);
        write_tag(ram, 176, 0, 7);

        retroshelf::SystemBus bus(ram);
        retroshelf::GsPacketProcessor parser;
        retroshelf::GsGifPipeline pipeline;
        retroshelf::GsFramebuffer framebuffer;
        const bool framebuffer_ready = framebuffer.resize(8, 8);
        retroshelf::EeGifDma dma(bus, [&](const auto& payload) {
            if (!parser.submit(payload)) throw std::runtime_error(parser.error());
        });
        dma.registers.chcr = Start | Direction | Chain;
        dma.registers.tadr = 0;
        const bool transferred = dma.start();
        const bool stream_finished = parser.finish_stream();
        const auto packets = parser.take_packets();
        check(transferred && stream_finished && packets.size() == 1,
              "chained EE GIF DMA transfers a complete five-register GIF stream");
        const bool rendered = framebuffer_ready &&
                              pipeline.consume_and_render(packets, framebuffer);
        check(rendered, "DMA packet renders through GIF parser and GS pipeline");
        retroshelf::GsFramebuffer::Rgba8 pixel{};
        check(rendered && framebuffer.get_pixel(2, 2, pixel) &&
                  pixel.r == 255 && pixel.g == 0 && pixel.b == 0 && pixel.a == 255,
              "end-to-end DMA-to-GS output contains the expected red triangle pixel");
    }

    return failures == 0 ? 0 : 1;
}
