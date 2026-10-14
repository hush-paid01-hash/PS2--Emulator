#pragma once

#include "retroshelf/gs_gif.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace retroshelf::gs {

// A view into a caller-owned array of 64-bit words. Each qword is [low, high].
// Payload remains opaque: parsing does not execute GS register writes or draw.
struct GifPacketView {
    GifTag tag{};
    std::size_t tag_qword_offset = 0;
    std::size_t payload_qword_offset = 0;
    std::size_t payload_qwords = 0;
};

// Parses complete packets until an EOP tag is reached. On malformed or truncated
// input, returns false and clears packets. Input storage remains caller-owned.
bool parse_gif_stream(const std::uint64_t* words,
                      std::size_t qword_count,
                      std::vector<GifPacketView>& packets,
                      std::size_t& consumed_qwords);

} // namespace retroshelf::gs
