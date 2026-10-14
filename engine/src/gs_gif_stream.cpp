#include "retroshelf/gs_gif_stream.hpp"

#include <limits>

namespace retroshelf::gs {

bool parse_gif_stream(const std::uint64_t* words,
                      std::size_t qword_count,
                      std::vector<GifPacketView>& packets,
                      std::size_t& consumed_qwords) {
    packets.clear();
    consumed_qwords = 0;
    if (words == nullptr || qword_count == 0) return false;

    std::size_t cursor = 0;
    while (cursor < qword_count) {
        if (qword_count - cursor < 1) {
            packets.clear();
            return false;
        }

        GifTag tag{};
        if (!GifTag::decode(words[cursor * 2], words[cursor * 2 + 1], tag)) {
            packets.clear();
            consumed_qwords = 0;
            return false;
        }

        const std::size_t payload = tag.payload_qwords();
        const std::size_t available = qword_count - cursor - 1;
        if (payload > available) {
            packets.clear();
            consumed_qwords = 0;
            return false;
        }

        GifPacketView view{};
        view.tag = tag;
        view.tag_qword_offset = cursor;
        view.payload_qword_offset = cursor + 1;
        view.payload_qwords = payload;
        packets.push_back(view);
        cursor += 1 + payload;

        if (tag.eop) {
            consumed_qwords = cursor;
            return true;
        }
    }

    // A stream without an EOP marker is incomplete; don't report partial packets.
    packets.clear();
    consumed_qwords = 0;
    return false;
}

} // namespace retroshelf::gs
