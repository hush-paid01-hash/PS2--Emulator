#include "retroshelf/ee_interrupts.hpp"
#include <iostream>

int main() {
    retroshelf::ee::EeInterrupts irq;
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << '\n'; ++failures; }
        else { std::cout << "PASS: " << label << '\n'; }
    };
    check(!irq.has_enabled_pending(), "no enabled interrupt after reset");
    irq.raise(1U << 2U);
    check(irq.pending() == (1U << 2U), "raising source sets pending bit");
    check(!irq.has_enabled_pending(), "masked pending source does not signal enabled interrupt");
    irq.set_mask(1U << 2U);
    check(irq.has_enabled_pending(), "unmasked pending source is signaled");
    irq.clear(1U << 2U);
    check(!irq.has_enabled_pending() && irq.pending() == 0, "clearing source removes pending bit");
    irq.raise(1U << 1U);
    irq.reset();
    check(irq.pending() == 0 && irq.mask() == 0, "reset clears pending and mask state");
    return failures == 0 ? 0 : 1;
}
