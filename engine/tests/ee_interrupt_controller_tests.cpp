#include "retroshelf/ee_interrupt_controller.hpp"

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

std::vector<std::uint8_t> word(std::uint32_t value) {
    return {
        static_cast<std::uint8_t>(value & 0xFFU),
        static_cast<std::uint8_t>((value >> 8U) & 0xFFU),
        static_cast<std::uint8_t>((value >> 16U) & 0xFFU),
        static_cast<std::uint8_t>((value >> 24U) & 0xFFU)
    };
}

std::uint32_t decode(const std::vector<std::uint8_t>& data) {
    if (data.size() != 4) return 0;
    return static_cast<std::uint32_t>(data[0]) |
           (static_cast<std::uint32_t>(data[1]) << 8U) |
           (static_cast<std::uint32_t>(data[2]) << 16U) |
           (static_cast<std::uint32_t>(data[3]) << 24U);
}

} // namespace

int main() {
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) {
            std::cerr << "FAIL: " << label << "\n";
            ++failures;
        } else {
            std::cout << "PASS: " << label << "\n";
        }
    };

    auto intc = std::make_shared<retroshelf::EeInterruptController>();
    std::vector<std::uint8_t> ram(64, 0);
    retroshelf::SystemBus bus(ram);
    std::string error;
    check(bus.map_mmio(retroshelf::EeInterruptController::RegisterBase,
                       retroshelf::EeInterruptController::RegisterSize, intc, &error),
          "maps EE INTC register block on SystemBus");

    check(intc->raise_interrupt(2) && intc->raise_interrupt(7),
          "device sources can assert independent interrupt lines");
    check(intc->status() == ((1U << 2U) | (1U << 7U)),
          "INTC_STAT records asserted lines");
    check(intc->mask() == 0 && intc->active_pending() == 0,
          "interrupts are masked after reset");

    std::vector<std::uint8_t> data;
    check(bus.read(retroshelf::EeInterruptController::RegisterBase, 4, data, &error) &&
              decode(data) == intc->status(),
          "SystemBus reads INTC_STAT");
    check(bus.write(retroshelf::EeInterruptController::RegisterBase + 0x10U,
                    word(1U << 2U), &error) && intc->mask() == (1U << 2U),
          "INTC_MASK write enables selected source by toggling its bit");
    check(intc->active_pending() == (1U << 2U),
          "active pending reports status bits enabled by mask");
    check(bus.write(retroshelf::EeInterruptController::RegisterBase + 0x10U,
                    word(1U << 2U), &error) && intc->mask() == 0,
          "writing the same INTC_MASK bit toggles it off");

    check(bus.write(retroshelf::EeInterruptController::RegisterBase,
                    word(1U << 2U), &error) &&
              intc->status() == (1U << 7U),
          "INTC_STAT write-one-to-clear clears only selected pending source");
    check(intc->clear_interrupt(7) && intc->status() == 0,
          "device can deassert a pending source");
    check(!intc->raise_interrupt(32) && !intc->clear_interrupt(32),
          "out-of-range interrupt lines are rejected");

    check(!bus.read(retroshelf::EeInterruptController::RegisterBase, 2, data, &error) &&
              data.empty(),
          "rejects non-32-bit register reads");
    check(!bus.write(retroshelf::EeInterruptController::RegisterBase + 4U,
                     word(0xFFFFFFFFU), &error),
          "rejects unimplemented INTC register writes");
    check(!bus.read(retroshelf::EeInterruptController::RegisterBase + 4U, 4, data, &error) &&
              data.empty(),
          "rejects unimplemented INTC register reads");
    check(!bus.write(retroshelf::EeInterruptController::RegisterBase + 1U,
                     word(1U), &error),
          "rejects unaligned INTC register writes");

    return failures == 0 ? 0 : 1;
}
