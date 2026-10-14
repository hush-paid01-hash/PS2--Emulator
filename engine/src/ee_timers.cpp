#include "retroshelf/ee_timers.hpp"
#include <algorithm>

namespace retroshelf::ee {
void EeTimer::reset() {
    count_ = target_ = 0; divisor_ = 1; remainder_ = 0;
    repeat_ = reached_ = event_ = false;
}
void EeTimer::configure(std::uint32_t target, std::uint32_t divisor, bool repeat) {
    target_ = target; divisor_ = std::max<std::uint32_t>(1, divisor);
    repeat_ = repeat; remainder_ = 0; reached_ = event_ = false;
}
void EeTimer::advance(std::uint64_t cycles) {
    remainder_ += cycles;
    const auto ticks = remainder_ / divisor_;
    remainder_ %= divisor_;
    for (std::uint64_t i = 0; i < ticks; ++i) {
        if (target_ != 0 && count_ >= target_) {
            reached_ = true; event_ = true;
            if (repeat_) count_ = 0;
            else { count_ = target_; return; }
        } else {
            ++count_;
            if (target_ != 0 && count_ >= target_) {
                reached_ = true; event_ = true;
                if (repeat_) count_ = 0;
                else return;
            }
        }
    }
}
std::uint32_t EeTimer::count() const { return count_; }
std::uint32_t EeTimer::target() const { return target_; }
bool EeTimer::reached_target() const { return reached_; }
bool EeTimer::take_target_event() { const bool value = event_; event_ = false; return value; }
} // namespace retroshelf::ee
