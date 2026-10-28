#include "retroshelf/gs_image_upload_stager.hpp"

namespace retroshelf {

bool GsImageUploadStager::stage(
    const GsTransferState& state,
    const std::vector<GsRegisterDecoder::ImageTransfer>& transfers,
    std::vector<std::uint8_t>& packed_bytes,
    std::string& error) const {
    error.clear();

    if (!state.accepts_image_upload()) {
        error = "GS transfer direction is not host-to-local";
        return false;
    }

    std::size_t expected = 0;
    if (!state.expected_image_bytes(expected)) {
        error = "GS transfer uses an unsupported pixel-storage mode";
        return false;
    }
    if (expected > MaxStagedBytes) {
        error = "GS image upload exceeds the staging limit";
        return false;
    }

    std::size_t supplied = 0;
    for (const auto& transfer : transfers) {
        if (transfer.format != 2U && transfer.format != 3U) {
            error = "Transfer list contains a non-IMAGE GIF format";
            return false;
        }
        if (transfer.data.size() > MaxStagedBytes - supplied) {
            error = "GS image payload exceeds the staging limit";
            return false;
        }
        supplied += transfer.data.size();
    }

    // GIF IMAGE payloads are qword-sized. Only the final transfer can contain
    // bytes beyond TRXREG's packed pixel count; allow at most 15 padding bytes.
    if (supplied < expected) {
        error = "GS image payload is shorter than TRXREG requires";
        return false;
    }
    if (supplied - expected >= 16U) {
        error = "GS image payload has more than final qword padding";
        return false;
    }

    std::vector<std::uint8_t> staged;
    staged.reserve(expected);
    for (const auto& transfer : transfers) {
        const std::size_t remaining = expected - staged.size();
        const std::size_t take = transfer.data.size() < remaining
            ? transfer.data.size() : remaining;
        staged.insert(staged.end(), transfer.data.begin(),
                      transfer.data.begin() + static_cast<std::ptrdiff_t>(take));
        if (staged.size() == expected) break;
    }

    packed_bytes.swap(staged);
    return true;
}

} // namespace retroshelf
