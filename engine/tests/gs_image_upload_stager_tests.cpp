#include "retroshelf/gs_image_upload_stager.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

int main() {
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << '\n'; ++failures; }
        else std::cout << "PASS: " << label << '\n';
    };

    retroshelf::GsTransferState state;
    state.write_register(0x50, 0x00ULL << 56U); // PSMCT32 destination
    state.write_register(0x52, 1ULL | (1ULL << 32U)); // 1x1 pixel
    state.write_register(0x53, 0);

    retroshelf::GsRegisterDecoder::ImageTransfer image{};
    image.format = 2;
    image.data = {0x11, 0x22, 0x33, 0x44, 0xAA, 0xBB, 0xCC, 0xDD,
                  0, 0, 0, 0, 0, 0, 0, 0};
    std::vector<retroshelf::GsRegisterDecoder::ImageTransfer> transfers{image};
    retroshelf::GsImageUploadStager stager;
    std::vector<std::uint8_t> output{0xFE};
    std::string error;

    check(stager.stage(state, transfers, output, error), "stages valid IMAGE payload");
    check(output == std::vector<std::uint8_t>({0x11, 0x22, 0x33, 0x44}),
          "strips final qword padding");
    check(error.empty(), "clears error on success");

    output = {0xFE};
    transfers[0].data.resize(3);
    check(!stager.stage(state, transfers, output, error), "rejects truncated image data");
    check(output == std::vector<std::uint8_t>({0xFE}), "failure preserves prior output");

    transfers[0].data.resize(32, 0);
    check(!stager.stage(state, transfers, output, error), "rejects excess beyond final qword padding");

    transfers[0].data.resize(16, 0);
    transfers[0].format = 0;
    check(!stager.stage(state, transfers, output, error), "rejects non-IMAGE format");

    transfers[0].format = 2;
    state.write_register(0x53, 1);
    check(!stager.stage(state, transfers, output, error), "rejects non-host-to-local direction");

    state.write_register(0x53, 0);
    state.write_register(0x50, 0x3FULL << 56U);
    check(!stager.stage(state, transfers, output, error), "rejects unsupported pixel format");

    return failures == 0 ? 0 : 1;
}
