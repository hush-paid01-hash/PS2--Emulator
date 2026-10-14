#pragma once

#include <cstdint>
#include <functional>
#include <queue>
#include <vector>

namespace retroshelf::ee {

class EeScheduler {
public:
    using Cycle = std::uint64_t;
    using Callback = std::function<void()>;

    EeScheduler();
    void reset();
    Cycle now() const;
    std::uint64_t schedule_after(Cycle delay, Callback callback);
    void advance(Cycle cycles);
    std::size_t pending_events() const;

private:
    struct Event {
        Cycle at;
        std::uint64_t id;
        Callback callback;
    };
    struct Later {
        bool operator()(const Event& a, const Event& b) const {
            return a.at != b.at ? a.at > b.at : a.id > b.id;
        }
    };
    Cycle now_{0};
    std::uint64_t next_id_{1};
    std::priority_queue<Event, std::vector<Event>, Later> events_;
};

} // namespace retroshelf::ee
