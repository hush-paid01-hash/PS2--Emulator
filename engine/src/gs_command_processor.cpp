#include "retroshelf/gs_command_processor.hpp"

namespace retroshelf {

bool GsCommandProcessor::submit_packed_packet(const std::vector<std::uint8_t>& packet) {
    error_.clear();
    if (!decoder_.decode_packed(packet)) {
        error_ = decoder_.error();
        return false;
    }

    // Decode fully before applying any writes so malformed packets do not
    // partially mutate GS state.
    for (const auto& write : decoder_.writes()) {
        if (!state_.write(write.address, write.value)) {
            error_ = "decoded GS register address is outside the register state";
            return false;
        }
        ++writes_processed_;
    }
    ++packets_processed_;
    return true;
}

const GsRegisterState& GsCommandProcessor::state() const { return state_; }
GsRegisterState& GsCommandProcessor::state() { return state_; }
const std::string& GsCommandProcessor::error() const { return error_; }
std::uint64_t GsCommandProcessor::packets_processed() const { return packets_processed_; }
std::uint64_t GsCommandProcessor::writes_processed() const { return writes_processed_; }

void GsCommandProcessor::reset() {
    decoder_.clear();
    state_.reset();
    error_.clear();
    packets_processed_ = 0;
    writes_processed_ = 0;
}

} // namespace retroshelf
