#pragma once
#include <cstdint>
#include <functional>
#include <string>

namespace retroshelf::ee {

// Prototype PS2 EE DMAC channel 2 (GIF), normal mode only. This models the
// channel register interface and synchronous transfer completion; timing,
// arbitration, stall control, and chained modes remain future work.
class EeGifDma {
public:
    static constexpr std::uint32_t Chcr = 0x1000A000U;
    static constexpr std::uint32_t Madr = 0x1000A010U;
    static constexpr std::uint32_t Qwc  = 0x1000A020U;
    static constexpr std::uint32_t Tadr = 0x1000A030U;
    static constexpr std::uint32_t Str  = 1U << 8U;
    using TransferHandler = std::function<bool(std::uint32_t, std::uint32_t, std::string*)>;

    void reset();
    void set_transfer_handler(TransferHandler handler);
    bool handles(std::uint32_t address) const;
    bool read32(std::uint32_t address, std::uint32_t& value) const;
    bool write32(std::uint32_t address, std::uint32_t value);
    const std::string& last_error() const;
private:
    bool start(std::string* error);
    std::uint32_t chcr_{0}, madr_{0}, qwc_{0}, tadr_{0};
    TransferHandler handler_;
    std::string last_error_;
};
} // namespace retroshelf::ee
