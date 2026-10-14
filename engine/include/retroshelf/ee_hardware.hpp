#pragma once

#include "retroshelf/ee_interrupts.hpp"
#include "retroshelf/ee_timers.hpp"
#include <cstdint>

namespace retroshelf::ee {

// Project-local MMIO prototype for tests; addresses are not the PS2 register map.
class EeHardware {
public:
    static constexpr std::uint32_t Timer0Interrupt = 1U << 0U;
    static constexpr std::uint32_t MmioBase = 0x10000000U;
    static constexpr std::uint32_t Timer0Count = MmioBase + 0x00U;
    static constexpr std::uint32_t Timer0Target = MmioBase + 0x04U;
    static constexpr std::uint32_t Timer0Control = MmioBase + 0x08U;
    static constexpr std::uint32_t InterruptPending = MmioBase + 0x10U;
    static constexpr std::uint32_t InterruptMask = MmioBase + 0x14U;

    void reset();
    void advance(std::uint64_t cycles);
    bool read32(std::uint32_t address, std::uint32_t& value) const;
    bool write32(std::uint32_t address, std::uint32_t value);
    EeTimer& timer0();
    const EeTimer& timer0() const;
    EeInterrupts& interrupts();
    const EeInterrupts& interrupts() const;
private:
    EeTimer timer0_;
    EeInterrupts interrupts_;
    std::uint32_t timer_control_{0};
};
} // namespace retroshelf::ee
