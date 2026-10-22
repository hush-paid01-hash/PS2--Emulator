#include "retroshelf/gs_register_decoder.hpp"
#include <cstdint>
#include <iostream>
#include <vector>

namespace {
void append_qword(std::vector<std::uint8_t>& bytes, std::uint64_t low, std::uint64_t high = 0) {
    for (unsigned i = 0; i < 8; ++i) bytes.push_back(static_cast<std::uint8_t>(low >> (i * 8U)));
    for (unsigned i = 0; i < 8; ++i) bytes.push_back(static_cast<std::uint8_t>(high >> (i * 8U)));
}
}
int main() {
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << "\n"; ++failures; }
        else std::cout << "PASS: " << label << "\n";
    };
    retroshelf::GsRegisterDecoder decoder;
    std::vector<std::uint8_t> packet;
    // NLOOP=2, EOP=1, PACKED, NREG=2, register list {0,1}.
    append_qword(packet, 2U | (1ULL << 15U) | (2ULL << 60U), 0x10U);
    append_qword(packet, 0x1234U); append_qword(packet, 0x5678U);
    append_qword(packet, 0x9ABCU); append_qword(packet, 0xDEF0U);
    check(decoder.decode_packed(packet), "decodes packed GIF register writes");
    check(decoder.writes().size() == 4, "emits each packed loop register write");
    if (decoder.writes().size() == 4) {
        check(decoder.writes()[0].address == 0 && decoder.writes()[1].address == 1,
              "decodes packed register list from tag high word");
        check(decoder.writes()[2].value == 0x9ABCU, "preserves ordered packed payload values");
    }

    std::vector<std::uint8_t> reglist;
    // NLOOP=1, EOP=1, REGLIST, NREG=3, register list {0,1,2}.
    // Three 64-bit values occupy two qwords; the last high half is padding.
    append_qword(reglist, 1U | (1ULL << 15U) | (1ULL << 58U) | (3ULL << 60U), 0x210U);
    append_qword(reglist, 0x111U, 0x222U);
    append_qword(reglist, 0x333U, 0xDEADU);
    check(decoder.decode_gif(reglist), "decodes REGLIST GIF tag");
    check(decoder.writes().size() == 3, "ignores unused high half of odd REGLIST payload");
    if (decoder.writes().size() == 3) {
        check(decoder.writes()[0].address == 0 && decoder.writes()[0].value == 0x111U,
              "decodes first REGLIST register/value");
        check(decoder.writes()[1].address == 1 && decoder.writes()[1].value == 0x222U,
              "decodes second REGLIST register/value");
        check(decoder.writes()[2].address == 2 && decoder.writes()[2].value == 0x333U,
              "decodes third REGLIST register/value");
    }

    std::vector<std::uint8_t> truncated;
    append_qword(truncated, 1U | (1ULL << 15U) | (1ULL << 60U), 0);
    check(!decoder.decode_gif(truncated), "rejects truncated packed payload");
    check(!decoder.error().empty(), "reports a useful decode error");

    std::vector<std::uint8_t> truncated_reglist;
    append_qword(truncated_reglist, 1U | (1ULL << 15U) | (1ULL << 58U) | (3ULL << 60U), 0x210U);
    append_qword(truncated_reglist, 0x111U, 0x222U);
    check(!decoder.decode_gif(truncated_reglist), "rejects truncated REGLIST payload");

    std::vector<std::uint8_t> unsupported;
    append_qword(unsupported, 1U | (1ULL << 15U) | (2ULL << 58U) | (1ULL << 60U), 0);
    check(!decoder.decode_gif(unsupported), "rejects unsupported image GIF format");
    return failures == 0 ? 0 : 1;
}
