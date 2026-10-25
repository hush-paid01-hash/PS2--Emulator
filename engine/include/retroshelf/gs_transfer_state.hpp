#pragma once

#include <cstddef>
#include <cstdint>

namespace retroshelf {

// Decodes the GS image-transfer setup registers. This deliberately does not
// write IMAGE payloads into VRAM: that requires the GS pixel swizzle tables.
class GsTransferState {
public:
    struct Snapshot {
        std::uint16_t source_base_256;
        std::uint8_t source_buffer_width_64;
        std::uint8_t source_pixel_storage;
        std::uint16_t destination_base_256;
        std::uint8_t destination_buffer_width_64;
        std::uint8_t destination_pixel_storage;
        std::uint16_t source_x;
        std::uint16_t source_y;
        std::uint16_t destination_x;
        std::uint16_t destination_y;
        bool reverse_x;
        bool reverse_y;
        std::uint16_t width;
        std::uint16_t height;
        std::uint8_t direction;
    };

    void reset();
    bool write_register(std::uint8_t address, std::uint64_t value);
    Snapshot snapshot() const;
    // Number of packed image bytes expected for TRXREG dimensions and PSM.
    // Returns false for unsupported PSM values or arithmetic overflow.
    bool expected_image_bytes(std::size_t& bytes) const;
    // Host-to-local transfers use TRXDIR=0. Other directions are not IMAGE uploads.
    bool accepts_image_upload() const;

private:
    std::uint64_t bitbltbuf_{0};
    std::uint64_t trxpos_{0};
    std::uint64_t trxreg_{0};
    std::uint64_t trxdir_{0};
};

} // namespace retroshelf
