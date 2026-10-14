#include "retroshelf/ee_machine.hpp"
#include <cstdint>
#include <iostream>
#include <string>
int main() {
    retroshelf::ee::EeMachine machine;
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << '\n'; ++failures; }
        else { std::cout << "PASS: " << label << '\n'; }
    };
    // Synthetic GIF PACKED packet: PRIM triangle-list, RGBAQ white, XYZ2 x3,
    // EOP set on the GIF tag. This verifies the actual MMIO-triggered path.
    const std::uint64_t stream[] = {
        1ULL | (1ULL << 60U), 0ULL, 3ULL, 0ULL,
        1ULL | (1ULL << 60U), 0x1ULL, 0xFFFFFFFFULL, 0x01ULL,
        3ULL | (1ULL << 15U) | (1ULL << 60U), 0x5ULL,
        (40ULL * 16ULL) | (20ULL * 16ULL << 16U), 0x05ULL,
        (280ULL * 16ULL) | (150ULL * 16ULL << 16U), 0x05ULL,
        (40ULL * 16ULL) | (150ULL * 16ULL << 16U), 0x05ULL
    };
    bool stored = true;
    for (std::size_t i = 0; i < sizeof(stream) / sizeof(stream[0]); ++i) {
        stored = stored && machine.load_word(0x2000U + static_cast<std::uint32_t>(i * 8U),
                                             static_cast<std::uint32_t>(stream[i]));
        stored = stored && machine.load_word(0x2004U + static_cast<std::uint32_t>(i * 8U),
                                             static_cast<std::uint32_t>(stream[i] >> 32U));
    }
    check(stored, "synthetic GIF packet stored in EE RAM");
    auto& bus = machine.system_bus();
    std::uint32_t value = 0;
    check(bus.write32(retroshelf::ee::EeGifDma::Madr, 0x2000U), "MADR register accepts source address");
    check(bus.write32(retroshelf::ee::EeGifDma::Qwc, 8U), "QWC register accepts qword count");
    check(bus.read32(retroshelf::ee::EeGifDma::Madr, value) && value == 0x2000U,
          "MADR register is visible through system bus");
    check(bus.read32(retroshelf::ee::EeGifDma::Qwc, value) && value == 8U,
          "QWC register is visible through system bus");
    check(bus.write32(retroshelf::ee::EeGifDma::Chcr, 1U | retroshelf::ee::EeGifDma::Str),
          "writing DIR and STR starts GIF normal-mode DMA");
    check(bus.read32(retroshelf::ee::EeGifDma::Chcr, value) && (value & retroshelf::ee::EeGifDma::Str) == 0,
          "STR clears when synchronous transfer completes");
    check(bus.read32(retroshelf::ee::EeGifDma::Qwc, value) && value == 0,
          "QWC reaches zero after completed transfer");
    check(bus.read32(retroshelf::ee::EeGifDma::Madr, value) && value == 0x2080U,
          "MADR advances by transferred qword bytes");
    check(machine.gs_renderer().pixels()[80U * machine.gs_renderer().width() + 100U] != 0x000000FFU, "MMIO DMA packet reaches renderer and draws a triangle");

    check(!bus.write32(retroshelf::ee::EeGifDma::Chcr, retroshelf::ee::EeGifDma::Str),
          "device-to-memory GIF direction is rejected");
    check(!bus.write32(retroshelf::ee::EeGifDma::Chcr, 1U | (1U << 2U) | retroshelf::ee::EeGifDma::Str),
          "GIF chain mode is rejected until implemented");
    check(bus.write32(retroshelf::ee::EeGifDma::Qwc, 0U), "QWC can be cleared");
    check(bus.write32(retroshelf::ee::EeGifDma::Madr, 0x2000U), "MADR can be reset for invalid-transfer test");
    check(!bus.write32(retroshelf::ee::EeGifDma::Chcr, 1U | retroshelf::ee::EeGifDma::Str),
          "zero-length transfer is rejected");
    check(bus.read32(retroshelf::ee::EeGifDma::Chcr, value) && (value & retroshelf::ee::EeGifDma::Str) == 0,
          "STR is cleared after rejected transfer");
    return failures == 0 ? 0 : 1;
}
