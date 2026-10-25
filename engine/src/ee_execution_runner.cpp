#include "retroshelf/ee_execution_runner.hpp"

namespace retroshelf {

EeExecutionRunner::EeExecutionRunner(Cpu& cpu, Ps2EeMemoryMap& memory)
    : cpu_(cpu), memory_(memory) {}

EeExecutionRunner::Result EeExecutionRunner::run(std::size_t instruction_budget) {
    Result result;
    result.final_pc = cpu_.pc();
    result.trace.reserve(instruction_budget < TraceCapacity ? instruction_budget : TraceCapacity);
    if (instruction_budget == 0) {
        result.reason = StopReason::NoInstructionsRequested;
        result.message = "Instruction budget is zero; no instructions executed.";
        return result;
    }

    for (std::size_t i = 0; i < instruction_budget; ++i) {
        const std::uint64_t instruction_pc = cpu_.pc();
        std::uint64_t raw_instruction = 0;
        std::string fetch_error;
        const bool fetched = instruction_pc <= 0xFFFFFFFFULL &&
            memory_.read(static_cast<std::uint32_t>(instruction_pc), 4, raw_instruction, &fetch_error);
        if (result.trace.size() < TraceCapacity) {
            result.trace.push_back({instruction_pc, static_cast<std::uint32_t>(raw_instruction), fetched});
        }

        std::string error;
        if (!cpu_.step_from_ee_memory(memory_, &error)) {
            result.instructions_executed = i;
            result.final_pc = cpu_.pc();
            result.message = error.empty() ? "EE instruction execution stopped." : error;
            result.reason = !fetched
                ? StopReason::InvalidInstructionFetch
                : ((cpu_.pc() == Cpu::GeneralExceptionVector)
                    ? StopReason::CpuExceptionOrUnsupportedInstruction
                    : StopReason::ExecutionError);
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
