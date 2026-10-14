#include "retroshelf/gs_vram.hpp"
#include <cstdint>
#include <iostream>

int main() {
    retroshelf::gs::GsVram vram;
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << '\n'; ++failures; }
        else std::cout << "PASS: " << label << '\n';
    };
    std::uint32_t value = 0;
    check(vram.bytes().size() == retroshelf::gs::GsVram::Size, "GS allocates 4 MiB VRAM");
    check(vram.write32(0, 0xFF332211U), "VRAM accepts aligned word writes");
    check(vram.read32(0, value) && value == 0xFF332211U, "VRAM word round-trip preserves RGBA bytes");
    check(!vram.read32(1, value), "VRAM rejects unaligned word reads");
    check(!vram.write32(retroshelf::gs::GsVram::Size, 0), "VRAM rejects out-of-range writes");
    check(vram.write_pixel_ct32(3, 2, 8, 0x80402010U), "CT32 pixel helper writes valid coordinate");
    check(vram.read_pixel_ct32(3, 2, 8, value) && value == 0x80402010U, "CT32 pixel helper reads written coordinate");
    check(!vram.write_pixel_ct32(8, 0, 8, 0), "CT32 pixel helper rejects x outside row");
    check(!vram.read_pixel_ct32(0, 0, 0, value), "CT32 pixel helper rejects zero-width surface");
    vram.reset();
    check(vram.read32(0, value) && value == 0, "VRAM reset clears contents");
    return failures == 0 ? 0 : 1;
}
