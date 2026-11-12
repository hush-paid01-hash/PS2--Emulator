#pragma once

#include "retroshelf/system_bus.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf {

// EE interrupt-controller register block. This device models the INTC STAT
// and MASK registers; CPU exception delivery and device interrupt sources
// are connected by the machine runtime, not by this register block itself.
class EeInterruptController final : public MmioDevice {
public:
    static constexpr std::uint32_t RegisterBase = 0x1000F000U;
    static constexpr std::uint32_t RegisterSize = 0x20U;
    static constexpr std::uint32_t StatOffset = 0x00U;
    static constexpr std::uint32_t MaskOffset = 0x10U;
    static constexpr std::uint32_t InterruptLineCount = 32U;

    bool read(std::uint32_t offset, std::size_t size,
              std::vector<std::uint8_t>& data,
              std::string* error = nullptr) const override;
    bool write(std::uint32_t offset, const std::vector<std::uint8_t>& data,
               std::string* error = nullptr) override;

    bool raise_interrupt(std::uint32_t line);
    bool clear_interrupt(std::uint32_t line);
    std::uint32_t status() const;
    std::uint32_t mask() const;
    std::uint32_t active_pending() const;

private:
    std::uint32_t status_{0};
    std::uint32_t mask_{0};
};

} // namespace retroshelf
