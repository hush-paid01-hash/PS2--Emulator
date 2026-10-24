#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "retroshelf/gs_register_decoder.hpp"
#include "retroshelf/gs_transfer_state.hpp"

namespace retroshelf {

// Validates and strips final GIF qword padding from host-to-local IMAGE data.
// This stages packed bytes only; it does not apply GS page/block swizzling or
// mutate local memory. That must happen in the pixel-format-aware transfer layer.
class GsImageUploadStager {
public:
    static constexpr std::size_t MaxStagedBytes = 16U * 1024U * 1024U;

    bool stage(const GsTransferState& state,
               const std::vector<GsRegisterDecoder::ImageTransfer>& transfers,
               std::vector<std::uint8_t>& packed_bytes,
               std::string& error) const;
};

} // namespace retroshelf
