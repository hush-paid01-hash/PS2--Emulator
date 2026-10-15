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
}

int main() {
    int failures = 0;
    const auto check = [&failures](bool ok, const char* name) {
        if (!ok) { std::cerr << "FAIL: " << name << '\n'; ++failures; }
        else std::cout << "PASS: " << name << '\n';
    };

    retroshelf::GsCommandProcessor gs;
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
    gs.reset();
    check(gs.state().read(1, value) && value == 0 && gs.packets_processed() == 0,
          "reset clears GS state and counters");
    return failures == 0 ? 0 : 1;
}
