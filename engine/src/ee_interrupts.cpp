#include "retroshelf/ee_interrupts.hpp"

namespace retroshelf::ee {

void EeInterrupts::reset() { pending_ = 0; mask_ = 0; }
void EeInterrupts::raise(std::uint32_t source_bit) { pending_ |= source_bit; }
void EeInterrupts::clear(std::uint32_t source_bit) { pending_ &= ~source_bit; }
void EeInterrupts::set_mask(std::uint32_t mask) { mask_ = mask; }
std::uint32_t EeInterrupts::pending() const { return pending_; }
std::uint32_t EeInterrupts::mask() const { return mask_; }
bool EeInterrupts::has_enabled_pending() const { return (pending_ & mask_) != 0; }

} // namespace retroshelf::ee
