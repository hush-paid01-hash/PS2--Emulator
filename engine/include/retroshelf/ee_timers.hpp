#pragma once
#include "retroshelf/ee_scheduler.hpp"
#include <cstdint>

namespace retroshelf::ee {

// Generic timer building block for tests. This is not yet the exact PS2 timer
// register map, clock divisors, or interrupt routing.
class EeTimer {
public:
    void reset();
    void configure(std::uint32_t target, std::uint32_t divisor = 1, bool repeat = false);
    void advance(std::uint64_t cycles);
    std::uint32_t count() const;
    std::uint32_t target() const;
    bool reached_target() const;
    bool take_target_event();
private:
    std::uint32_t count_{0};
    std::uint32_t target_{0};
    std::uint32_t divisor_{1};
    std::uint64_t remainder_{0};
    bool repeat_{false};
    bool reached_{false};
    bool event_{false};
};
} // namespace retroshelf::ee
