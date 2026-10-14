#include "retroshelf/gs_gif.hpp"
#include <cstdint>
#include <iostream>

int main() {
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << '\\n'; ++failures; }
        else std::cout << "PASS: " << label << '\\n';
    };
    retroshelf::gs::GifTag tag{};
    // NLOOP=3, EOP=1, FLG=PACKED(0), NREG=2; register IDs 0x1,0xE.
    const std::uint64_t low = 3U | (1ULL << 15U) | (2ULL << 60U);
    const std::uint64_t high = 0xE1ULL;
    check(retroshelf::gs::GifTag::decode(low, high, tag), "decodes a valid packed GIFtag");
    check(tag.nloop == 3 && tag.eop && tag.format == 0 && tag.nreg == 2,
          "extracts NLOOP, EOP, FLG and NREG");
    check(tag.registers[0] == 1 && tag.registers[1] == 14,
          "extracts packed register descriptor nibbles");
    const std::uint64_t nreg_zero = 0;
    check(retroshelf::gs::GifTag::decode(nreg_zero, 0, tag) && tag.nreg == 16,
          "interprets NREG zero as sixteen registers");
    const std::uint64_t reserved = 3ULL << 58U;
    check(!retroshelf::gs::GifTag::decode(reserved, 0, tag),
          "rejects reserved GIF format without changing to a false success");
    return failures == 0 ? 0 : 1;
}
