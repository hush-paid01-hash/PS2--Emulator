#include "retroshelf/ee_machine.hpp"
#include "retroshelf/ps2_elf.hpp"
#include <cstdint>
#include <iostream>
#include <vector>
#include <string>

namespace {
void put16(std::vector<std::uint8_t>& b, std::size_t o, std::uint16_t v) {
    b[o] = static_cast<std::uint8_t>(v); b[o + 1] = static_cast<std::uint8_t>(v >> 8U);
}
void put32(std::vector<std::uint8_t>& b, std::size_t o, std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i) b[o + i] = static_cast<std::uint8_t>(v >> (i * 8U));
}
std::vector<std::uint8_t> elf_fixture() {
    std::vector<std::uint8_t> b(0x104, 0);
    b[0] = 0x7f; b[1] = 'E'; b[2] = 'L'; b[3] = 'F'; b[4] = 1; b[5] = 1; b[6] = 1;
    put16(b, 16, 2); put16(b, 18, 8); put32(b, 20, 1);
    put32(b, 24, 0x100); put32(b, 28, 52); put16(b, 40, 52); put16(b, 42, 32); put16(b, 44, 1);
    put32(b, 52, 1); put32(b, 56, 0x100); put32(b, 60, 0x100);
    put32(b, 68, 4); put32(b, 72, 8); put32(b, 76, 5);
    b[0x100] = 0x07; b[0x101] = 0x00; b[0x102] = 0x08; b[0x103] = 0x25; // addiu t0, zero, 7
    return b;
}
}
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
    machine.hardware().write32(retroshelf::ee::EeHardware::Timer0Target, 3);
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
    check(!machine.hardware().interrupts().has_enabled_pending(), "timer interrupt remains clear before third cycle");
    check(machine.step(&error), "machine executes third instruction");
    check(machine.scheduler().now() == 3, "third step advances scheduler");
    check(machine.hardware().interrupts().has_enabled_pending(), "machine step raises interrupt when timer reaches target");
    machine.reset();
    constexpr std::uint32_t set_base = (0x09U << 26U) | (8U << 16U) | 0x0100U;
    constexpr std::uint32_t set_value = (0x09U << 26U) | (9U << 16U) | 0x1234U;
    constexpr std::uint32_t store_word = (0x2BU << 26U) | (8U << 21U) | (9U << 16U);
    constexpr std::uint32_t load_word = (0x23U << 26U) | (8U << 21U) | (10U << 16U);
    check(machine.load_word(0, set_base), "integrated program loads base-address instruction");
    check(machine.load_word(4, set_value), "integrated program loads value instruction");
    check(machine.load_word(8, store_word), "integrated program loads store instruction");
    check(machine.load_word(12, load_word), "integrated program loads load instruction");
    check(machine.step(&error) && machine.step(&error) && machine.step(&error) && machine.step(&error),
          "machine runs four-instruction RAM store/load program");
    std::uint32_t stored = 0;
    check(machine.bus().read32(0x100, stored) && stored == 0x1234,
          "integrated store writes expected RAM value");
    check(machine.cpu().gpr(10).lo == 0x1234,
          "integrated load returns stored RAM value into CPU register");
    machine.reset();
    std::uint32_t word = 0;
    const auto parsed_elf = retroshelf::media::parse_ps2_elf(elf_fixture());
    check(parsed_elf.valid, "integration fixture parses as ELF");
    check(machine.load_elf(parsed_elf, &error), "machine loads validated ELF into RAM");
    check(machine.cpu().pc() == 0x100, "ELF loader sets CPU entry point");
    check(machine.step(&error), "machine executes first instruction from loaded ELF entry point");
    check(machine.cpu().gpr(8).lo == 7, "loaded ELF instruction updates CPU register");
    check(machine.bus().read32(0x104, word) && word == 0, "ELF loader zero-fills BSS in RAM");
    machine.load_word(0x200, 0xAABBCCDDU);
    auto bad_elf = parsed_elf;
    bad_elf.segments[0].virtual_address = static_cast<std::uint32_t>(retroshelf::ee::EeBus::RamSize - 2);
    check(!machine.load_elf(bad_elf, &error), "ELF segment extending beyond RAM is rejected");
    check(machine.bus().read32(0x200, word) && word == 0xAABBCCDDU,
          "rejected ELF leaves existing machine RAM untouched");
    machine.reset();
    word = 1;
    check(machine.cpu().pc() == 0, "machine reset resets PC");
    check(machine.bus().read32(0, word) && word == 0, "machine reset clears RAM");
    check(machine.scheduler().now() == 0, "machine reset resets scheduler");
    check(machine.hardware().interrupts().pending() == 0, "machine reset clears hardware interrupt state");
    check(machine.step(&error), "zeroed RAM contains a supported SLL-zero instruction");
    check(machine.cpu().pc() == 4, "machine advances after zero instruction");
    return failures == 0 ? 0 : 1;
}
