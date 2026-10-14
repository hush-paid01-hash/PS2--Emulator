#include "retroshelf/ee_timers.hpp"
#include <iostream>
int main() {
    retroshelf::ee::EeTimer timer; int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << '\n'; ++failures; }
        else { std::cout << "PASS: " << label << '\n'; }
    };
    timer.configure(3, 2, false); timer.advance(3);
    check(timer.count() == 1, "divisor converts cycles into timer ticks");
    timer.advance(3);
    check(timer.count() == 3 && timer.reached_target(), "timer reaches target");
    check(timer.take_target_event(), "target event is reported once");
    check(!timer.take_target_event(), "target event clears when consumed");
    timer.reset(); timer.configure(2, 1, true); timer.advance(5);
    check(timer.count() == 1 && timer.reached_target(), "repeat timer wraps at target");
    check(timer.take_target_event(), "repeat timer reports target event");
    return failures == 0 ? 0 : 1;
}
