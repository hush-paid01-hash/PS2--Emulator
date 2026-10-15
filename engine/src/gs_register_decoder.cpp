#include "retroshelf/gs_register_decoder.hpp"

#include <limits>

namespace retroshelf {
namespace {
constexpr std::size_t QwordSize = 16;
constexpr std::size_t MaxWrites = 4 * 1024 * 1024;

std::uint64_t read_u64(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < 8; ++i)
        value |= static_cast<std::uint64_t>(bytes[offset + i]) << (i * 8U);
    return value;
}
} // namespace

bool GsRegisterDecoder::fail(const std::string& message) {
    error_ = message;
    return false;
}

bool GsRegisterDecoder::decode_packed(const std::vector<std::uint8_t>& packet) {
    clear();
    if (packet.size() % QwordSize != 0U)
        return fail("GS packet is not qword-aligned");

    std::size_t offset = 0;
    while (offset < packet.size()) {
        if (packet.size() - offset < QwordSize)
            return fail("GS packet ends before a GIF tag is complete");

        const std::uint64_t tag = read_u64(packet, offset);
        const std::uint64_t register_list = read_u64(packet, offset + 8U);
        const std::size_t loops = static_cast<std::size_t>(tag & 0x7FFFU);
        const bool eop = ((tag >> 15U) & 1U) != 0U;
        const std::uint8_t format = static_cast<std::uint8_t>((tag >> 58U) & 3U);
        const std::size_t raw_nreg = static_cast<std::size_t>((tag >> 60U) & 0xFU);
        const std::size_t nreg = raw_nreg == 0U ? 16U : raw_nreg;

        if (format != 0U)
            return fail("GS register decoder currently supports PACKED GIF tags only");
        offset += QwordSize;

        if (loops != 0U && nreg > std::numeric_limits<std::size_t>::max() / loops)
            return fail("GIF packed register count overflows");
        const std::size_t values = loops * nreg;
        if (values > (packet.size() - offset) / QwordSize)
            return fail("GIF packed tag payload is truncated");
        if (values > MaxWrites - writes_.size())
            return fail("GS register write limit exceeded");

        for (std::size_t i = 0; i < values; ++i) {
            const std::size_t slot = i % nreg;
            const auto address = static_cast<std::uint8_t>((register_list >> (slot * 4U)) & 0xFU);
            writes_.push_back({address, read_u64(packet, offset)});
            offset += QwordSize;
        }
        if (eop && offset != packet.size())
            return fail("GIF EOP occurs before the end of the supplied packet");
    }
    return true;
}

const std::vector<GsRegisterDecoder::Write>& GsRegisterDecoder::writes() const {
    return writes_;
}
const std::string& GsRegisterDecoder::error() const { return error_; }
void GsRegisterDecoder::clear() {
    writes_.clear();
    error_.clear();
}
} // namespace retroshelf
