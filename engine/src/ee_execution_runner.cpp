#include "retroshelf/ee_execution_runner.hpp"

namespace retroshelf {

EeExecutionRunner::EeExecutionRunner(Cpu& cpu, Ps2EeMemoryMap& memory)
    : cpu_(cpu), memory_(memory) {}

EeExecutionRunner::Result EeExecutionRunner::run(std::size_t instruction_budget) {
    Result result;
    result.final_pc = cpu_.pc();
    if (instruction_budget == 0) {
        result.message = "Instruction budget is zero; no instructions executed.";
        return result;
    }

    for (std::size_t i = 0; i < instruction_budget; ++i) {
        std::string error;
        if (!cpu_.step_from_ee_memory(memory_, &error)) {
            result.instructions_executed = i;
            result.final_pc = cpu_.pc();
            result.message = error.empty() ? "EE instruction execution stopped." : error;
            result.reason = (cpu_.pc() == Cpu::GeneralExceptionVector)
                ? StopReason::CpuExceptionOrUnsupportedInstruction
                : StopReason::InvalidInstructionFetch;
            return result;
        }
    }

    result.instructions_executed = instruction_budget;
    result.final_pc = cpu_.pc();
    result.reason = StopReason::InstructionLimit;
    result.message = "Instruction budget reached; execution paused safely.";
    return result;
}

} // namespace retroshelf
