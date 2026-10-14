#include "retroshelf/ps2_elf.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

namespace {
void put16(std::vector<std::uint8_t>& b, std::size_t o, std::uint16_t v) {
    b[o] = static_cast<std::uint8_t>(v); b[o + 1] = static_cast<std::uint8_t>(v >> 8U);
}
void put32(std::vector<std::uint8_t>& b, std::size_t o, std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i) b[o + i] = static_cast<std::uint8_t>(v >> (i * 8U));
}
std::vector<std::uint8_t> fixture() {
    std::vector<std::uint8_t> b(0x104, 0);
    b[0] = 0x7f; b[1] = 'E'; b[2] = 'L'; b[3] = 'F';
    b[4] = 1; b[5] = 1; b[6] = 1;
    put16(b, 16, 2); put16(b, 18, 8); put32(b, 20, 1);
    put32(b, 24, 0x00100000); put32(b, 28, 52);
    put16(b, 40, 52); put16(b, 42, 32); put16(b, 44, 1);
    put32(b, 52, 1); put32(b, 56, 0x100); put32(b, 60, 0x00100000);
    put32(b, 68, 4); put32(b, 72, 8); put32(b, 76, 5); put32(b, 80, 0x1000);
    b[0x100] = 0x11; b[0x101] = 0x22; b[0x102] = 0x33; b[0x103] = 0x44;
    return b;
}
}
int main() {
    int failures = 0;
    const auto check = [&failures](bool ok, const char* name) {
        if (!ok) { std::cerr << "FAIL: " << name << '\n'; ++failures; }
        else std::cout << "PASS: " << name << '\n';
    };
    auto image = fixture();
    auto parsed = retroshelf::media::parse_ps2_elf(image);
    check(parsed.valid, "accepts valid ELF32 little-endian MIPS executable");
    check(parsed.entry_point == 0x00100000, "reads ELF entry point");
    check(parsed.segments.size() == 1, "extracts PT_LOAD segment");
    check(parsed.segments.size() == 1 && parsed.segments[0].bytes.size() == 8,
          "allocates segment memory size");
    check(parsed.segments.size() == 1 && parsed.segments[0].bytes[0] == 0x11
          && parsed.segments[0].bytes[3] == 0x44, "copies segment file bytes");
    check(parsed.segments.size() == 1 && parsed.segments[0].bytes[4] == 0
          && parsed.segments[0].bytes[7] == 0, "zero-fills BSS tail");
    check(!retroshelf::media::parse_ps2_elf({}).valid, "rejects truncated ELF");
    image[0] = 0;
    check(!retroshelf::media::parse_ps2_elf(image).valid, "rejects bad ELF magic");
    image = fixture(); put16(image, 18, 3);
    check(!retroshelf::media::parse_ps2_elf(image).valid, "rejects non-MIPS ELF");
    image = fixture(); put32(image, 68, 9);
    check(!retroshelf::media::parse_ps2_elf(image).valid, "rejects file size greater than memory size");
    image = fixture(); put32(image, 56, 0x200000);
    check(!retroshelf::media::parse_ps2_elf(image).valid, "rejects unmapped entry point");
    image = fixture(); put32(image, 56, 0x100); put32(image, 68, 0xfffffff0U);
    check(!retroshelf::media::parse_ps2_elf(image).valid, "rejects segment extending beyond file");
    return failures == 0 ? 0 : 1;
}
