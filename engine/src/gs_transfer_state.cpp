#include "retroshelf/gs_transfer_state.hpp"

#include <limits>

namespace retroshelf {

void GsTransferState::reset() {
    bitbltbuf_ = 0;
    trxpos_ = 0;
    trxreg_ = 0;
    trxdir_ = 0;
}

bool GsTransferState::write_register(std::uint8_t address, std::uint64_t value) {
    switch (address) {
        case 0x50: bitbltbuf_ = value; return true; // BITBLTBUF
        case 0x51: trxpos_ = value; return true;    // TRXPOS
        case 0x52: trxreg_ = value; return true;    // TRXREG
        case 0x53: trxdir_ = value; return true;    // TRXDIR
        default: return false;
    }
}

GsTransferState::Snapshot GsTransferState::snapshot() const {
    Snapshot s{};
    s.source_base_256 = static_cast<std::uint16_t>(bitbltbuf_ & 0x3FFFU);
    s.source_buffer_width_64 = static_cast<std::uint8_t>((bitbltbuf_ >> 16U) & 0x3FU);
    s.source_pixel_storage = static_cast<std::uint8_t>((bitbltbuf_ >> 24U) & 0x3FU);
    s.destination_base_256 = static_cast<std::uint16_t>((bitbltbuf_ >> 32U) & 0x3FFFU);
    s.destination_buffer_width_64 = static_cast<std::uint8_t>((bitbltbuf_ >> 48U) & 0x3FU);
    s.destination_pixel_storage = static_cast<std::uint8_t>((bitbltbuf_ >> 56U) & 0x3FU);
    s.source_x = static_cast<std::uint16_t>(trxpos_ & 0x7FFU);
    s.source_y = static_cast<std::uint16_t>((trxpos_ >> 16U) & 0x7FFU);
    s.destination_x = static_cast<std::uint16_t>((trxpos_ >> 32U) & 0x7FFU);
    s.destination_y = static_cast<std::uint16_t>((trxpos_ >> 48U) & 0x7FFU);
    s.reverse_x = ((trxpos_ >> 59U) & 1U) != 0U;
    s.reverse_y = ((trxpos_ >> 60U) & 1U) != 0U;
    s.width = static_cast<std::uint16_t>(trxreg_ & 0xFFFU);
    s.height = static_cast<std::uint16_t>((trxreg_ >> 32U) & 0xFFFU);
    s.direction = static_cast<std::uint8_t>(trxdir_ & 3U);
    return s;
}

bool GsTransferState::expected_image_bytes(std::size_t& bytes) const {
    const Snapshot s = snapshot();
    std::size_t bits_per_pixel = 0;
    switch (s.destination_pixel_storage) {
        case 0x00: case 0x30: bits_per_pixel = 32; break; // CT32/Z32
        case 0x01: case 0x31: bits_per_pixel = 24; break; // CT24/Z24
        case 0x02: case 0x0A: case 0x32: case 0x3A: bits_per_pixel = 16; break;
        case 0x13: case 0x1B: bits_per_pixel = 8; break;  // T8/T8H
        case 0x14: case 0x24: case 0x2C: bits_per_pixel = 4; break;
        default: return false;
    }
    const std::size_t pixels = static_cast<std::size_t>(s.width) * s.height;
    if (pixels > (std::numeric_limits<std::size_t>::max() - 7U) / bits_per_pixel)
        return false;
    bytes = (pixels * bits_per_pixel + 7U) / 8U;
    return true;
}

bool GsTransferState::accepts_image_upload() const {
    return snapshot().direction == 0U;
}

} // namespace retroshelf
