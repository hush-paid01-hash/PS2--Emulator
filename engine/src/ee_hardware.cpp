#include "retroshelf/ee_hardware.hpp"

namespace retroshelf::ee {
void EeHardware::reset() { timer0_.reset(); interrupts_.reset(); }
void EeHardware::advance(std::uint64_t cycles) {
    timer0_.advance(cycles);
    if (timer0_.take_target_event()) interrupts_.raise(Timer0Interrupt);
}
EeTimer& EeHardware::timer0() { return timer0_; }
const EeTimer& EeHardware::timer0() const { return timer0_; }
EeInterrupts& EeHardware::interrupts() { return interrupts_; }
const EeInterrupts& EeHardware::interrupts() const { return interrupts_; }
} // namespace retroshelf::ee
