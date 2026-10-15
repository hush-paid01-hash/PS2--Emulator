#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf {

struct GifPacket {
    std::vector<std::uint8_t> bytes;
    std::size_t tag_count{0};
};

class GsPacketProcessor {
public:
    static constexpr std::size_t QwordSize = 16;
    static constexpr std::size_t MaxPacketBytes = 64 * 1024 * 1024;
    static constexpr std::size_t MaxQueuedPacketBytes = 64 * 1024 * 1024;

    bool submit(const std::vector<std::uint8_t>& data);
    bool finish_stream();
    std::vector<GifPacket> take_packets();
    const std::string& error() const;

private:
    bool fail(const std::string& message);
    bool complete_packet();

    std::vector<std::uint8_t> input_;
    std::vector<std::uint8_t> current_packet_;
    std::vector<GifPacket> completed_packets_;
    std::size_t queued_packet_bytes_{0};
    std::size_t body_qwords_remaining_{0};
    std::size_t current_packet_tags_{0};
    bool packet_ends_at_body_{false};
    bool finished_{false};
    bool failed_{false};
    std::string error_;
};

} // namespace retroshelf
