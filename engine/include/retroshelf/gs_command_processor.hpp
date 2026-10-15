#pragma once

#include "retroshelf/gs_register_decoder.hpp"
#include "retroshelf/gs_register_state.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf {

// Connects decoded PACKED GIF register writes to the GS register state.
// Non-PACKED GIF modes remain explicitly unsupported by GsRegisterDecoder.
class GsCommandProcessor {
public:
    bool submit_packed_packet(const std::vector<std::uint8_t>& packet);
    const GsRegisterState& state() const;
    GsRegisterState& state();
    const std::string& error() const;
    std::uint64_t packets_processed() const;
    std::uint64_t writes_processed() const;
    void reset();

private:
    GsRegisterDecoder decoder_;
    GsRegisterState state_;
    std::string error_;
    std::uint64_t packets_processed_{0};
    std::uint64_t writes_processed_{0};
};

} // namespace retroshelf
