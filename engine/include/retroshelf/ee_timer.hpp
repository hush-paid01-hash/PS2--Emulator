#pragma once

#include "retroshelf/system_bus.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf {

// Register-level model of the four EE hardware timers. Time advances only
// when the machine runtime explicitly supplies EE bus cycles; external
// H/V-blank clock sources and gate inputs are intentionally not synthesized.
class EeTimerBlock final : public MmioDevice {
public:
    static constexpr std::uint32_t RegisterBase = 0x10000000U;
    static constexpr std::uint32_t RegisterSize = 0x4000U;
    static constexpr std::uint32_t ChannelStride = 0x800U;
    static constexpr std::uint32_t CountOffset = 0x00U;
    static constexpr std::uint32_t ModeOffset = 0x10U;
    static constexpr std::uint32_t CompareOffset = 0x20U;
    static constexpr std::uint32_t HoldOffset = 0x30U;
    static constexpr std::uint32_t ChannelCount = 4U;

    static constexpr std::uint32_t ModeClockMask = 0x3U;
    static constexpr std::uint32_t ModeGate = 1U << 2U;
    static constexpr std::uint32_t ModeGateSourceShift = 3U;
    static constexpr std::uint32_t ModeGateSourceMask = 0x3U << ModeGateSourceShift;
    static constexpr std::uint32_t ModeGateMode = 1U << 5U;
    static constexpr std::uint32_t ModeZeroReturn = 1U << 6U;
    static constexpr std::uint32_t ModeCountEnable = 1U << 7U;
    static constexpr std::uint32_t ModeCompareEnable = 1U << 8U;
    static constexpr std::uint32_t ModeOverflowEnable = 1U << 9U;
    static constexpr std::uint32_t ModeCompareFlag = 1U << 10U;
    static constexpr std::uint32_t ModeOverflowFlag = 1U << 11U;

    bool read(std::uint32_t offset, std::size_t size,
              std::vector<std::uint8_t>& data,
              std::string* error = nullptr) const override;
    bool write(std::uint32_t offset, const std::vector<std::uint8_t>& data,
               std::string* error = nullptr) override;

    // Advance using EE bus cycles. CLKS=0 uses every supplied cycle,
    // CLKS=1 divides by 16, CLKS=2 divides by 256, CLKS=3 is external.
    void advance_cycles(std::uint64_t cycles);
    std::uint16_t count(std::uint32_t channel) const;
    std::uint16_t compare(std::uint32_t channel) const;
    std::uint32_t mode(std::uint32_t channel) const;
    std::uint32_t pending_interrupts() const;

private:
    struct Timer {
        std::uint16_t count{0};
        std::uint16_t compare{0};
        std::uint16_t hold{0};
        std::uint32_t mode{0};
        std::uint64_t prescale_remainder{0};
    };
    std::array<Timer, ChannelCount> timers_{};
};

} // namespace retroshelf
