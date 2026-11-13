#include "retroshelf/ps2_disc_execution.hpp"
#include "retroshelf/ee_interrupt_controller.hpp"
#include "retroshelf/ee_timer.hpp"
#include "retroshelf/system_bus.hpp"

#include <memory>

namespace retroshelf {

Ps2DiscExecutionResult load_and_execute_ps2_disc(const Iso9660Disc& disc,
                                                  Ps2EeMemoryMap& memory,
                                                  Cpu& cpu,
                                                  std::size_t instruction_budget) {
    Ps2DiscExecutionResult result;
    if (!boot_ps2_disc_into_ee_memory(disc, memory, result.executable_path,
                                      result.entry_point, &result.error)) {
        result.execution.final_pc = cpu.pc();
        return result;
    }

    result.image_loaded = true;
    cpu.reset();
    cpu.set_pc(result.entry_point);
    // Give the guest CPU shared MMIO access to INTC and EE timers.
    SystemBus bus(memory);
    auto intc = std::make_shared<EeInterruptController>();
    auto timers = std::make_shared<EeTimerBlock>();
    if (!bus.map_mmio(EeInterruptController::RegisterBase,
                      EeInterruptController::RegisterSize, intc, &result.error)) {
        result.image_loaded = false;
        return result;
    }
    if (!bus.map_mmio(EeTimerBlock::RegisterBase, EeTimerBlock::RegisterSize,
                      timers, &result.error)) {
        result.image_loaded = false;
        return result;
    }
    EeExecutionRunner runner(cpu, memory, bus, *intc, *timers);
    result.execution = runner.run(instruction_budget);
    return result;
}

} // namespace retroshelf
