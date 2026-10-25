#include "retroshelf/gs_transfer_state.hpp"

#include <cstdint>
#include <iostream>
#include <limits>

int main() {
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << '\n'; ++failures; }
        else std::cout << "PASS: " << label << '\n';
    };

    retroshelf::GsTransferState transfer;
    check(transfer.write_register(0x50, 0x0000000000000000ULL), "accepts BITBLTBUF");
    check(transfer.write_register(0x51, 0), "accepts TRXPOS");
    check(transfer.write_register(0x52, 0), "accepts TRXREG");
    check(transfer.write_register(0x53, 0), "accepts TRXDIR");
    check(!transfer.write_register(0x54, 0), "rejects unrelated GS register");

    const std::uint64_t bitblt =
        0x123ULL | (0x20ULL << 16U) | (0x00ULL << 24U) |
        (0x234ULL << 32U) | (0x10ULL << 48U) | (0x02ULL << 56U);
    const std::uint64_t trxpos =
        17ULL | (29ULL << 16U) | (41ULL << 32U) | (53ULL << 48U) |
        (1ULL << 59U);
    const std::uint64_t trxreg = 64ULL | (32ULL << 32U);
    check(transfer.write_register(0x50, bitblt), "stores BITBLTBUF fields");
    check(transfer.write_register(0x51, trxpos), "stores TRXPOS fields");
    check(transfer.write_register(0x52, trxreg), "stores TRXREG dimensions");
    check(transfer.write_register(0x53, 0), "stores host-to-local TRXDIR");
    const auto s = transfer.snapshot();
    check(s.source_base_256 == 0x123 && s.source_buffer_width_64 == 0x20 &&
              s.source_pixel_storage == 0, "decodes source BITBLTBUF fields");
    check(s.destination_base_256 == 0x234 && s.destination_buffer_width_64 == 0x10 &&
              s.destination_pixel_storage == 2, "decodes destination BITBLTBUF fields");
    check(s.source_x == 17 && s.source_y == 29 &&
              s.destination_x == 41 && s.destination_y == 53 && s.reverse_x,
          "decodes TRXPOS coordinates and direction");
    check(s.width == 64 && s.height == 32, "decodes TRXREG width and height");
    std::size_t bytes = 0;
    check(transfer.expected_image_bytes(bytes) && bytes == 64U * 32U * 2U,
          "calculates CT16 transfer byte count");

    transfer.write_register(0x50, 0x00ULL << 56U);
    check(transfer.expected_image_bytes(bytes) && bytes == 64U * 32U * 4U,
          "calculates CT32 transfer byte count");
    transfer.write_register(0x50, 0x13ULL << 56U);
    check(transfer.expected_image_bytes(bytes) && bytes == 64U * 32U,
          "calculates T8 transfer byte count");
    transfer.write_register(0x50, 0x14ULL << 56U);
    check(transfer.expected_image_bytes(bytes) && bytes == (64U * 32U) / 2U,
          "calculates T4 transfer byte count");

    check(transfer.accepts_image_upload(), "accepts host-to-local direction");
    transfer.write_register(0x53, 1);
    check(!transfer.accepts_image_upload(), "does not treat local-to-host as IMAGE upload");
    transfer.write_register(0x50, 0x3FULL << 56U);
    check(!transfer.expected_image_bytes(bytes), "rejects unsupported pixel-storage modes");
    transfer.reset();
    check(transfer.snapshot().width == 0 && transfer.snapshot().height == 0 &&
              transfer.accepts_image_upload(), "reset clears transfer setup");
    return failures == 0 ? 0 : 1;
}
