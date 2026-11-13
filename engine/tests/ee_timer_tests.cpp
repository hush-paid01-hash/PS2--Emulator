#include "retroshelf/ee_timer.hpp"
#include "retroshelf/ee_execution_runner.hpp"
#include "retroshelf/ee_interrupt_controller.hpp"
#include "retroshelf/cpu.hpp"
#include "retroshelf/ps2_ee_memory_map.hpp"

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

    // Verify the full timer -> INTC -> COP0 interrupt path while executing EE instructions.
    retroshelf::Ps2EeMemoryMap ee_memory(64U * 1024U);
    retroshelf::SystemBus ee_bus(ee_memory);
    auto runtime_timers = std::make_shared<retroshelf::EeTimerBlock>();
    auto intc = std::make_shared<retroshelf::EeInterruptController>();
    check(ee_bus.map_mmio(retroshelf::EeTimerBlock::RegisterBase,
                          retroshelf::EeTimerBlock::RegisterSize, runtime_timers, &error) &&
              ee_bus.map_mmio(retroshelf::EeInterruptController::RegisterBase,
                              retroshelf::EeInterruptController::RegisterSize, intc, &error),
          "maps timers and INTC into the EE execution bus");
    check(ee_bus.write(retroshelf::EeTimerBlock::RegisterBase +
                       retroshelf::EeTimerBlock::CompareOffset, word(2), &error) &&
              ee_bus.write(retroshelf::EeTimerBlock::RegisterBase +
                       retroshelf::EeTimerBlock::ModeOffset,
                       word(retroshelf::EeTimerBlock::ModeCountEnable |
                            retroshelf::EeTimerBlock::ModeCompareEnable), &error) &&
              ee_bus.write(retroshelf::EeInterruptController::RegisterBase +
                       retroshelf::EeInterruptController::MaskOffset, word(1U << 9U), &error),
          "arms timer 0 compare interrupt and unmasks its INTC line");
    constexpr std::uint32_t program = 0x80001000U;
    const std::uint32_t instructions[] = {
        0x24080401U, // ADDIU t0, zero, 0x401 (COP0 IE + IM2)
        0x40886000U, // MTC0 t0, Status
        0x00000000U  // Interrupt must be accepted before this NOP
    };
    for (std::size_t i = 0; i < 3U; ++i)
        check(ee_memory.write(program + static_cast<std::uint32_t>(i * 4U),
                              4U, instructions[i], &error),
              "writes timer interrupt test instruction");
    check(ee_memory.write(static_cast<std::uint32_t>(retroshelf::Cpu::GeneralExceptionVector),
                          4U, 0U, &error), "maps EE general exception vector");
    retroshelf::Cpu ee_cpu;
    ee_cpu.set_pc(program);
    retroshelf::EeExecutionRunner runtime_runner(ee_cpu, ee_memory, ee_bus, *intc,
                                                  *runtime_timers);
    const auto runtime_result = runtime_runner.run(3U);
    check(runtime_result.trace.size() == 3U &&
              runtime_result.trace[2].exception_taken &&
              runtime_result.trace[2].exception_code == 0U,
          "timer compare reaches COP0 as an EE external interrupt");
    check((intc->status() & (1U << 9U)) != 0U &&
              (ee_cpu.cop0_cause() & retroshelf::Cpu::CauseEeInterruptPending) != 0U &&
              ee_cpu.cop0_epc() == program + 8U,
          "timer IRQ is latched in INTC and records the interrupted PC");

    return failures == 0 ? 0 : 1;
}
