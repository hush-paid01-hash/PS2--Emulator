#include "retroshelf/ps2_disc_execution.hpp"

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
    EeExecutionRunner runner(cpu, memory);
    result.execution = runner.run(instruction_budget);
    return result;
}

} // namespace retroshelf
