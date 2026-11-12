#include "retroshelf/ee_timer.hpp"

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {
std::vector<std::uint8_t> word(std::uint32_t v) {
    return {static_cast<std::uint8_t>(v), static_cast<std::uint8_t>(v >> 8U),
            static_cast<std::uint8_t>(v >> 16U), static_cast<std::uint8_t>(v >> 24U)};
}
std::uint32_t decode(const std::vector<std::uint8_t>& d) {
    return static_cast<std::uint32_t>(d[0]) |
        (static_cast<std::uint32_t>(d[1]) << 8U) |
        (static_cast<std::uint32_t>(d[2]) << 16U) |
        (static_cast<std::uint32_t>(d[3]) << 24U);
}
}

int main() {
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << "\n"; ++failures; }
        else std::cout << "PASS: " << label << "\n";
    };
    auto timers = std::make_shared<retroshelf::EeTimerBlock>();
    std::vector<std::uint8_t> ram(64, 0);
    retroshelf::SystemBus bus(ram);
    std::string error;
    check(bus.map_mmio(retroshelf::EeTimerBlock::RegisterBase,
                       retroshelf::EeTimerBlock::RegisterSize, timers, &error),
          "maps all four EE timer register channels");
    const auto base = retroshelf::EeTimerBlock::RegisterBase;

    timers->advance_cycles(100);
    check(timers->count(0) == 0, "timer remains stopped after reset");
    check(bus.write(base + retroshelf::EeTimerBlock::CompareOffset, word(3), &error) &&
              bus.write(base + retroshelf::EeTimerBlock::ModeOffset,
                        word(retroshelf::EeTimerBlock::ModeCountEnable |
                             retroshelf::EeTimerBlock::ModeCompareEnable |
                             retroshelf::EeTimerBlock::ModeZeroReturn), &error),
          "programs compare, zero-return and count-enable");
    timers->advance_cycles(3);
    check(timers->count(0) == 0 &&
              (timers->mode(0) & retroshelf::EeTimerBlock::ModeCompareFlag) != 0U,
          "compare match raises EQUF and zero-return resets COUNT");
    check(timers->pending_interrupts() == 1U,
          "enabled compare flag exposes timer interrupt request");

    check(bus.write(base + retroshelf::EeTimerBlock::ModeOffset,
                    word(retroshelf::EeTimerBlock::ModeCountEnable |
                         retroshelf::EeTimerBlock::ModeCompareEnable |
                         retroshelf::EeTimerBlock::ModeZeroReturn |
                         retroshelf::EeTimerBlock::ModeCompareFlag), &error) &&
              timers->pending_interrupts() == 0U,
          "writing EQUF clears compare status without disabling control bits");

    const auto ch1 = base + retroshelf::EeTimerBlock::ChannelStride;
    check(bus.write(ch1 + retroshelf::EeTimerBlock::ModeOffset,
                    word(retroshelf::EeTimerBlock::ModeCountEnable |
                         (1U << 0U)), &error),
          "configures independent divided-clock channel");
    timers->advance_cycles(15);
    check(timers->count(1) == 0, "divide-by-16 timer does not tick early");
    timers->advance_cycles(1);
    check(timers->count(1) == 1, "divide-by-16 timer advances on sixteenth bus cycle");

    check(bus.write(ch1 + retroshelf::EeTimerBlock::CountOffset, word(0xFFFF), &error),
          "loads 16-bit timer counter");
    timers->advance_cycles(16);
    check(timers->count(1) == 0 &&
              (timers->mode(1) & retroshelf::EeTimerBlock::ModeOverflowFlag) != 0U,
          "counter wrap sets OVFF");
    std::vector<std::uint8_t> invalid_read;
    check(!bus.read(base + 4U, 4, invalid_read, &error) && invalid_read.empty(),
          "rejects unimplemented register offsets");
    check(!bus.write(base + 1U, word(1), &error),
          "rejects unaligned timer register writes");
    check(timers->count(4) == 0, "out-of-range channel query is safe");
    return failures == 0 ? 0 : 1;
}
