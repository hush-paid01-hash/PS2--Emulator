#include "retroshelf/gs_packet_processor.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace retroshelf {
namespace {

std::uint64_t read_u64(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    std::uint64_t value = 0;
    for (std::size_t byte = 0; byte < 8; ++byte) {
        value |= static_cast<std::uint64_t>(bytes[offset + byte]) << (byte * 8U);
    }
    return value;
}

} // namespace

bool GsPacketProcessor::fail(const std::string& message) {
    failed_ = true;
    error_ = message;
    return false;
}

void GsPacketProcessor::complete_packet() {
    completed_packets_.push_back({std::move(current_packet_), current_packet_tags_});
    current_packet_.clear();
    current_packet_tags_ = 0;
}

bool GsPacketProcessor::submit(const std::vector<std::uint8_t>& data) {
    if (failed_) {
        return false;
    }
    if (data.size() % QwordSize != 0U) {
        return fail("GIF input is not qword-aligned");
    }

    input_.insert(input_.end(), data.begin(), data.end());
    std::size_t consumed = 0;
    while (true) {
        const std::size_t available_qwords = (input_.size() - consumed) / QwordSize;
        if (body_qwords_remaining_ != 0U) {
            if (available_qwords == 0U) {
                break;
            }
            const std::size_t qwords =
                std::min(available_qwords, body_qwords_remaining_);
            const std::size_t bytes = qwords * QwordSize;
            if (bytes > MaxPacketBytes - current_packet_.size()) {
                return fail("GIF packet exceeds the size limit");
            }
            current_packet_.insert(current_packet_.end(), input_.begin() + consumed,
                                   input_.begin() + consumed + bytes);
            consumed += bytes;
            body_qwords_remaining_ -= qwords;
            if (body_qwords_remaining_ != 0U) {
                break;
            }
            if (packet_ends_at_body_) {
                complete_packet();
            }
            continue;
        }

        if (available_qwords == 0U) {
            break;
        }
        if (current_packet_.size() > MaxPacketBytes - QwordSize) {
            return fail("GIF packet exceeds the size limit");
        }

        const std::uint64_t low = read_u64(input_, consumed);
        const std::uint64_t high = read_u64(input_, consumed + 8U);
        const auto loops = static_cast<std::size_t>(low & 0x7FFFU);
        const bool eop = ((low >> 15U) & 1U) != 0U;
        const auto format = static_cast<std::uint8_t>((low >> 58U) & 3U);
        const auto register_count_raw = static_cast<std::size_t>((low >> 60U) & 0xFU);
        const std::size_t register_count =
            register_count_raw == 0U ? 16U : register_count_raw;

        std::size_t body_qwords = 0;
        switch (format) {
        case 0:
            body_qwords = loops * register_count;
            break;
        case 1: {
            const std::size_t register_values = loops * register_count;
            body_qwords = register_values / 2U + register_values % 2U;
            break;
        }
        case 2:
            body_qwords = loops;
            break;
        default:
            return fail("GIF tag uses the reserved transfer format");
        }

        if (body_qwords > (MaxPacketBytes - current_packet_.size() - QwordSize) /
                              QwordSize) {
            return fail("GIF packet exceeds the size limit");
        }

        current_packet_.insert(current_packet_.end(), input_.begin() + consumed,
                               input_.begin() + consumed + QwordSize);
        consumed += QwordSize;
        ++current_packet_tags_;
        body_qwords_remaining_ = body_qwords;
        packet_ends_at_body_ = eop;
        if (body_qwords_remaining_ == 0U && packet_ends_at_body_) {
            complete_packet();
        }
        (void)high;
    }

    if (consumed != 0U) {
        input_.erase(input_.begin(), input_.begin() + consumed);
    }
    return true;
}

bool GsPacketProcessor::finish_stream() {
    if (failed_) {
        return false;
    }
    if (!input_.empty() || body_qwords_remaining_ != 0U || !current_packet_.empty()) {
        return fail("GIF stream ended with an incomplete packet");
    }
    return true;
}

std::vector<GifPacket> GsPacketProcessor::take_packets() {
    auto packets = std::move(completed_packets_);
    completed_packets_.clear();
    return packets;
}

const std::string& GsPacketProcessor::error() const { return error_; }

} // namespace retroshelf
