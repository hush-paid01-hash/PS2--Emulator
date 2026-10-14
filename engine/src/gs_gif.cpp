#include "retroshelf/gs_gif.hpp"

namespace retroshelf::gs {

bool GifTag::decode(std::uint64_t low, std::uint64_t high, GifTag& out) {
    GifTag decoded{};
    decoded.nloop = static_cast<std::uint16_t>(low & 0x7FFFU);
    decoded.eop = ((low >> 15U) & 1U) != 0;
    decoded.format = static_cast<std::uint8_t>((low >> 58U) & 0x3U);
    const auto encoded_nreg = static_cast<std::uint8_t>((low >> 60U) & 0xFU);
    decoded.nreg = encoded_nreg == 0 ? 16 : encoded_nreg;
    for (std::uint8_t i = 0; i < decoded.nreg; ++i) {
        decoded.registers[i] = static_cast<std::uint8_t>((high >> (i * 4U)) & 0xFU);
    }
    // GIF FLG=3 is reserved; reject it rather than misinterpreting payloads.
    if (decoded.format == 3) return false;
    out = decoded;
    return true;
}

} // namespace retroshelf::gs
