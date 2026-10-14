#include "retroshelf/ee_machine.hpp"
#include <cstdint>
#include <iostream>
#include <string>

int main() {
    retroshelf::ee::EeMachine machine;
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << '\n'; ++failures; }
        else { std::cout << "PASS: " << label << '\n'; }
    };
    constexpr std::uint32_t addiu_t0_7 = (0x09U << 26U) | (8U << 16U) | 7U;
    constexpr std::uint32_t addiu_t1_minus1 = (0x09U << 26U) | (9U << 16U) | 0xFFFFU;
    check(machine.load_word(0, addiu_t0_7), "program word loads into RAM");
    check(machine.load_word(4, addiu_t1_minus1), "second program word loads into RAM");
    machine.hardware().write32(retroshelf::ee::EeHardware::Timer0Target, 2);
    machine.hardware().write32(retroshelf::ee::EeHardware::Timer0Control, 1);
    machine.hardware().interrupts().set_mask(retroshelf::ee::EeHardware::Timer0Interrupt);
    std::string error;
    check(machine.step(&error), "machine fetches and executes first instruction");
    check(machine.cpu().gpr(8).lo == 7 && machine.cpu().pc() == 4, "first instruction updates CPU state");
    check(machine.scheduler().now() == 1, "machine advances placeholder scheduler cycle");
    check(!machine.hardware().interrupts().has_enabled_pending(), "timer interrupt stays clear before target");
    check(machine.step(&error), "machine fetches and executes second instruction");
    check(machine.cpu().gpr(9).lo == 0xFFFFFFFFFFFFFFFFULL, "immediate is sign-extended in prototype");
    check(machine.cpu().pc() == 8, "machine advances PC after each instruction");
    check(machine.scheduler().now() == 2, "second step advances scheduler");
    check(machine.hardware().interrupts().has_enabled_pending(), "machine step advances timer and raises interrupt at target");
    machine.reset();
    std::uint32_t word = 1;
    check(machine.cpu().pc() == 0, "machine reset resets PC");
    check(machine.bus().read32(0, word) && word == 0, "machine reset clears RAM");
    check(machine.scheduler().now() == 0, "machine reset resets scheduler");
    check(machine.hardware().interrupts().pending() == 0, "machine reset clears hardware interrupt state");
    check(machine.step(&error), "zeroed RAM contains a supported SLL-zero instruction");
    check(machine.cpu().pc() == 4, "machine advances after zero instruction");
    return failures == 0 ? 0 : 1;
}
