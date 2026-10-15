#include "retroshelf/gs_command_processor.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

namespace {
void append_qword(std::vector<std::uint8_t>& out, std::uint64_t low, std::uint64_t high = 0) {
    for (unsigned i = 0; i < 8; ++i) out.push_back(static_cast<std::uint8_t>(low >> (i * 8U)));
    for (unsigned i = 0; i < 8; ++i) out.push_back(static_cast<std::uint8_t>(high >> (i * 8U)));
}
std::uint64_t packed_tag(std::uint16_t loops, bool eop, std::uint8_t nreg) {
    return static_cast<std::uint64_t>(loops) |
           (static_cast<std::uint64_t>(eop) << 15U) |
           (static_cast<std::uint64_t>(nreg) << 60U);
}
std::uint64_t xyz(std::uint16_t x, std::uint16_t y) {
    return static_cast<std::uint64_t>(x) | (static_cast<std::uint64_t>(y) << 16U);
}
}

int main() {
    int failures = 0;
    const auto check = [&failures](bool ok, const char* name) {
        if (!ok) { std::cerr << "FAIL: " << name << '\n'; ++failures; }
        else std::cout << "PASS: " << name << '\n';
    };

    retroshelf::GsCommandProcessor gs(8, 8);
    std::vector<std::uint8_t> packet;
    // PACKED tag: one loop, two register descriptors: address 1 then address 2.
    append_qword(packet, packed_tag(1, true, 2), 0x21ULL);
    append_qword(packet, 0x1122334455667788ULL);
    append_qword(packet, 0x8877665544332211ULL);
    check(gs.submit_packed_packet(packet), "valid packed packet is applied");
    std::uint64_t value = 0;
    check(gs.state().read(1, value) && value == 0x1122334455667788ULL,
          "first decoded write reaches GS register state");
    check(gs.state().read(2, value) && value == 0x8877665544332211ULL,
          "second decoded write reaches GS register state");
    check(gs.packets_processed() == 1 && gs.writes_processed() == 2,
          "packet and write counters track successful work");

    std::vector<std::uint8_t> malformed;
    append_qword(malformed, packed_tag(2, true, 1), 1);
    append_qword(malformed, 0x55);
    check(!gs.submit_packed_packet(malformed), "truncated packet is rejected");
    check(gs.state().read(1, value) && value == 0x1122334455667788ULL,
          "invalid packet does not partially apply writes");
    check(gs.packets_processed() == 1 && gs.writes_processed() == 2,
          "failed packet does not increment successful counters");

    // One PACKED GIF tag emits PRIM, RGBAQ, then three XYZ2 vertices.
    std::vector<std::uint8_t> triangle;
    append_qword(triangle, packed_tag(5, true, 1), 0x510U); // NREG=1, XYZ2
    // Use a separate tag with the register descriptor sequence PRIM/RGBAQ/XYZ2.
    triangle.clear();
    append_qword(triangle, packed_tag(5, true, 3), 0x510U);
    append_qword(triangle, 3U);              // PRIM: triangle list
    append_qword(triangle, 0xFF0000FFU);     // RGBAQ: opaque red
    append_qword(triangle, xyz(16, 16));     // (1,1)
    append_qword(triangle, xyz(96, 16));     // (6,1)
    append_qword(triangle, xyz(16, 96));     // (1,6)
    // NREG=3 descriptor order repeats: PRIM, RGBAQ, XYZ2, then PRIM...
    // The sequence above must be encoded as a single descriptor per vertex
    // cycle, so build a proper tag with NREG=3 and two loops: this also
    // exercises the repeated descriptor sequence.
    triangle.clear();
    append_qword(triangle, packed_tag(2, true, 3), 0x510U);
    append_qword(triangle, 3U);              // PRIM
    append_qword(triangle, 0xFF0000FFU);     // RGBAQ
    append_qword(triangle, xyz(16, 16));     // first vertex
    append_qword(triangle, 3U);
    append_qword(triangle, 0xFF0000FFU);
    append_qword(triangle, xyz(96, 16));     // second vertex
    // NREG=3 can't represent an arbitrary register stream; send the third
    // vertex in a second valid tag with PRIM/RGBAQ/XYZ2 descriptors.
    triangle.clear();
    append_qword(triangle, packed_tag(1, false, 3), 0x510U);
    append_qword(triangle, 3U);
    append_qword(triangle, 0xFF0000FFU);
    append_qword(triangle, xyz(16, 16));
    append_qword(triangle, packed_tag(2, true, 1), 0x5U);
    append_qword(triangle, xyz(96, 16));
    append_qword(triangle, xyz(16, 96));
    check(gs.submit_packed_packet(triangle), "packed GS primitive commands are accepted");
    retroshelf::GsFramebuffer::Pixel pixel{};
    check(gs.framebuffer().get_pixel(2, 2, pixel) &&
          pixel == retroshelf::GsFramebuffer::Pixel{255, 0, 0, 255},
          "GS PRIM/RGBAQ/XYZ2 commands draw a colored triangle");
    check(gs.triangles_rendered() == 1, "rendered triangle counter increments");

    gs.reset();
    check(gs.state().read(1, value) && value == 0 && gs.packets_processed() == 0,
          "reset clears GS state and counters");
    check(gs.triangles_rendered() == 0, "reset clears rendered triangle counter");
    return failures == 0 ? 0 : 1;
}
