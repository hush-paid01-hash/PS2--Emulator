#pragma once

#include "retroshelf/ee_interrupts.hpp"
#include "retroshelf/ee_timers.hpp"
#include <cstdint>

namespace retroshelf::ee {

// Testable integration layer; register addresses and interrupt semantics are
// placeholders until verified against PS2 hardware documentation.
class EeHardware {
public:
    static constexpr std::uint32_t Timer0Interrupt = 1U << 0U;
    void reset();
    void advance(std::uint64_t cycles);
    EeTimer& timer0();
    const EeTimer& timer0() const;
    EeInterrupts& interrupts();
    const EeInterrupts& interrupts() const;
private:
    EeTimer timer0_;
    EeInterrupts interrupts_;
};
} // namespace retroshelf::ee
