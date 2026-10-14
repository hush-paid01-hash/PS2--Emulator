#include "retroshelf/gs_packet_processor.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

namespace {

void append_qword(std::vector<std::uint8_t>& bytes, std::uint64_t low,
                  std::uint64_t high = 0) {
    for (std::size_t byte = 0; byte < 8; ++byte) {
        bytes.push_back(static_cast<std::uint8_t>(low >> (byte * 8U)));
    }
    for (std::size_t byte = 0; byte < 8; ++byte) {
        bytes.push_back(static_cast<std::uint8_t>(high >> (byte * 8U)));
    }
}

std::uint64_t make_tag(std::uint16_t loops, bool eop, std::uint8_t format,
                       std::uint8_t registers) {
    return loops | (static_cast<std::uint64_t>(eop) << 15U) |
           (static_cast<std::uint64_t>(format) << 58U) |
           (static_cast<std::uint64_t>(registers) << 60U);
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
        retroshelf::GsPacketProcessor processor;
        std::vector<std::uint8_t> header;
        append_qword(header, make_tag(1, true, 0, 1), 0x1234);
        check(processor.submit(header), "packed GIF tag header is accepted");
        check(processor.take_packets().empty(),
              "packet is not emitted before its packed payload arrives");

        std::vector<std::uint8_t> body;
        append_qword(body, 0x1122334455667788ULL, 0x99AABBCCDDEEFF00ULL);
        check(processor.submit(body) && processor.finish_stream(),
              "packed packet completes across DMA payload boundaries");
        auto packets = processor.take_packets();
        std::vector<std::uint8_t> expected = header;
        expected.insert(expected.end(), body.begin(), body.end());
        check(packets.size() == 1 && packets[0].bytes == expected &&
                  packets[0].tag_count == 1,
              "packed packet preserves its exact bytes and tag count");
    }

    {
        retroshelf::GsPacketProcessor processor;
        std::vector<std::uint8_t> stream;
        append_qword(stream, make_tag(1, true, 1, 3));
        append_qword(stream, 0x0102030405060708ULL, 0x1112131415161718ULL);
        append_qword(stream, 0x2122232425262728ULL, 0x3132333435363738ULL);
        check(processor.submit(stream) && processor.finish_stream(),
              "reglist tag consumes rounded-up pairs of 64-bit values");
        const auto packets = processor.take_packets();
        check(packets.size() == 1 && packets[0].bytes == stream,
              "reglist packet retains source data");
    }

    {
        retroshelf::GsPacketProcessor processor;
        std::vector<std::uint8_t> stream;
        append_qword(stream, make_tag(2, true, 2, 0));
        append_qword(stream, 0x4142434445464748ULL);
        append_qword(stream, 0x5152535455565758ULL);
        check(processor.submit(stream) && processor.finish_stream(),
              "image tag consumes one qword per loop");
        const auto packets = processor.take_packets();
        check(packets.size() == 1 && packets[0].bytes == stream,
              "image packet retains source data");
    }

    {
        retroshelf::GsPacketProcessor processor;
        std::vector<std::uint8_t> stream;
        append_qword(stream, make_tag(1, true, 3, 0));
        append_qword(stream, 0x6162636465666768ULL, 0x7172737475767778ULL);
        check(processor.submit(stream) && processor.finish_stream(),
              "IMAGE2 tag consumes image payload qwords");
        const auto packets = processor.take_packets();
        check(packets.size() == 1 && packets[0].bytes == stream,
              "IMAGE2 packet retains source data");
    }

    {
        retroshelf::GsPacketProcessor processor;
        std::vector<std::uint8_t> stream;
        append_qword(stream, make_tag(0, false, 0, 1));
        append_qword(stream, make_tag(0, true, 0, 1));
        check(processor.submit(stream) && processor.finish_stream(),
              "multiple tags are grouped until EOP");
        const auto packets = processor.take_packets();
        check(packets.size() == 1 && packets[0].bytes == stream &&
                  packets[0].tag_count == 2,
              "EOP packet preserves ordered tag headers");
    }

    {
        retroshelf::GsPacketProcessor processor;
        std::vector<std::uint8_t> incomplete;
        append_qword(incomplete, make_tag(1, true, 0, 1));
        check(processor.submit(incomplete) && !processor.finish_stream() &&
                  processor.error().find("incomplete") != std::string::npos,
              "end of stream reports a truncated GIF payload");
    }

    {
        retroshelf::GsPacketProcessor processor;
        check(!processor.submit(std::vector<std::uint8_t>{1, 2, 3}),
              "non-qword GIF input is rejected");
    }

    return failures == 0 ? 0 : 1;
}
