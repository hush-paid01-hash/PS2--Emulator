#include "retroshelf/ee_scheduler.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace retroshelf::ee {

EeScheduler::EeScheduler() = default;

void EeScheduler::reset() {
    now_ = 0;
    next_id_ = 1;
    events_ = {};
}

EeScheduler::Cycle EeScheduler::now() const { return now_; }

std::uint64_t EeScheduler::schedule_after(Cycle delay, Callback callback) {
    if (!callback) throw std::invalid_argument("EE scheduler callback must not be empty");
    if (delay > std::numeric_limits<Cycle>::max() - now_)
        throw std::overflow_error("EE scheduler cycle overflow");
    const auto id = next_id_++;
    events_.push(Event{now_ + delay, id, std::move(callback)});
    return id;
}

void EeScheduler::advance(Cycle cycles) {
    if (cycles > std::numeric_limits<Cycle>::max() - now_)
        throw std::overflow_error("EE scheduler cycle overflow");
    const auto target = now_ + cycles;
    while (!events_.empty() && events_.top().at <= target) {
        auto event = events_.top();
        events_.pop();
        now_ = event.at;
        event.callback();
    }
    now_ = target;
}

std::size_t EeScheduler::pending_events() const { return events_.size(); }

} // namespace retroshelf::ee
