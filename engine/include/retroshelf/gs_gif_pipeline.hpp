#pragma once

#include <string>
#include <vector>

#include "retroshelf/gs_packet_processor.hpp"
#include "retroshelf/gs_register_decoder.hpp"
#include "retroshelf/gs_register_state.hpp"

namespace retroshelf {

// Connects complete GIF packets to the current GS PACKED-tag register model.
// REGLIST and IMAGE payloads remain explicitly unsupported by this first pipeline.
class GsGifPipeline {
public:
    bool consume(const std::vector<GifPacket>& packets);
    const GsRegisterState& registers() const;
    GsRegisterState& registers();
    const std::string& error() const;
    void reset();

private:
    GsRegisterDecoder decoder_;
    GsRegisterState registers_;
    std::string error_;
};

} // namespace retroshelf
