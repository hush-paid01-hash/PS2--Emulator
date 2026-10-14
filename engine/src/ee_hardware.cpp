#include "retroshelf/ee_hardware.hpp"

namespace retroshelf::ee {
void EeHardware::reset() { timer0_.reset(); interrupts_.reset(); timer_control_ = 0; }
void EeHardware::advance(std::uint64_t cycles) {
    if ((timer_control_ & 1U) == 0U) return;
    timer0_.advance(cycles);
    if (timer0_.take_target_event()) interrupts_.raise(Timer0Interrupt);
}
bool EeHardware::read32(std::uint32_t address, std::uint32_t& value) const {
    switch (address) {
        case Timer0Count: value = timer0_.count(); return true;
        case Timer0Target: value = timer0_.target(); return true;
        case Timer0Control: value = timer_control_; return true;
        case InterruptPending: value = interrupts_.pending(); return true;
        case InterruptMask: value = interrupts_.mask(); return true;
        default: return false;
    }
}
bool EeHardware::write32(std::uint32_t address, std::uint32_t value) {
    switch (address) {
        case Timer0Count:
            timer0_.reset();
            timer0_.configure(value, (timer_control_ & 2U) ? 2U : 1U, (timer_control_ & 4U) != 0U);
            return true;
        case Timer0Target:
            timer0_.configure(value, (timer_control_ & 2U) ? 2U : 1U, (timer_control_ & 4U) != 0U);
            return true;
        case Timer0Control:
            timer_control_ = value & 7U;
            timer0_.configure(timer0_.target(), (timer_control_ & 2U) ? 2U : 1U, (timer_control_ & 4U) != 0U);
            return true;
        case InterruptPending: interrupts_.clear(value); return true;
        case InterruptMask: interrupts_.set_mask(value); return true;
        default: return false;
    }
}
EeTimer& EeHardware::timer0() { return timer0_; }
const EeTimer& EeHardware::timer0() const { return timer0_; }
EeInterrupts& EeHardware::interrupts() { return interrupts_; }
const EeInterrupts& EeHardware::interrupts() const { return interrupts_; }
} // namespace retroshelf::ee
