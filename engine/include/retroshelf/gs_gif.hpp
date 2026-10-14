#pragma once

#include <array>
#include <cstdint>

namespace retroshelf::gs {

// Decoded 128-bit PS2 GIFtag. This validates and describes packet headers;
// it does not yet execute GIF payloads or rasterize GS primitives.
struct GifTag {
    std::uint16_t nloop = 0;
    bool eop = false;
    std::uint8_t format = 0;
    std::uint8_t nreg = 0;
    std::array<std::uint8_t, 16> registers{};

    static bool decode(std::uint64_t low, std::uint64_t high, GifTag& out);

    // Number of 128-bit payload units following this tag, excluding the tag itself.
    std::uint32_t payload_qwords() const;
    std::uint32_t packet_qwords() const;
};

} // namespace retroshelf::gs
