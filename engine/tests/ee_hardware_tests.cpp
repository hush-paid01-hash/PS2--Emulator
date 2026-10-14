#include "retroshelf/ee_hardware.hpp"
#include <iostream>
int main() {
    retroshelf::ee::EeHardware hardware;
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << '\n'; ++failures; }
        else { std::cout << "PASS: " << label << '\n'; }
    };
    hardware.timer0().configure(3, 1, false);
    hardware.interrupts().set_mask(retroshelf::ee::EeHardware::Timer0Interrupt);
    hardware.advance(2);
    check(!hardware.interrupts().has_enabled_pending(), "timer does not interrupt before target");
    hardware.advance(1);
    check(hardware.interrupts().has_enabled_pending(), "timer target raises enabled interrupt");
    check((hardware.interrupts().pending() & retroshelf::ee::EeHardware::Timer0Interrupt) != 0,
          "timer interrupt source is pending");
    hardware.interrupts().clear(retroshelf::ee::EeHardware::Timer0Interrupt);
    check(!hardware.interrupts().has_enabled_pending(), "clearing interrupt acknowledges event");
    hardware.reset();
    check(hardware.interrupts().pending() == 0, "hardware reset clears interrupt state");
    return failures == 0 ? 0 : 1;
}
