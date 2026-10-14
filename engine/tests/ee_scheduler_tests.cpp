#include "retroshelf/ee_scheduler.hpp"
#include <iostream>
#include <vector>

int main() {
    retroshelf::ee::EeScheduler scheduler;
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << '\n'; ++failures; }
        else { std::cout << "PASS: " << label << '\n'; }
    };
    std::vector<int> fired;
    scheduler.schedule_after(5, [&] { fired.push_back(2); });
    scheduler.schedule_after(2, [&] { fired.push_back(1); });
    scheduler.schedule_after(5, [&] { fired.push_back(3); });
    check(scheduler.now() == 0, "scheduler starts at cycle zero");
    scheduler.advance(2);
    check(fired.size() == 1 && fired[0] == 1, "earlier event fires at correct cycle");
    check(scheduler.now() == 2, "cycle counter advances");
    scheduler.advance(3);
    check(fired.size() == 3 && fired[1] == 2 && fired[2] == 3, "same-cycle events preserve insertion order");
    check(scheduler.pending_events() == 0, "completed events are removed");
    scheduler.reset();
    check(scheduler.now() == 0 && scheduler.pending_events() == 0, "reset clears clock and events");
    return failures == 0 ? 0 : 1;
}
