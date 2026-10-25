#include "retroshelf/ee_execution_runner.hpp"

namespace retroshelf {

EeExecutionRunner::EeExecutionRunner(Cpu& cpu, Ps2EeMemoryMap& memory)
    : cpu_(cpu), memory_(memory) {}

EeExecutionRunner::Result EeExecutionRunner::run_from_bios_reset_vector(std::size_t instruction_budget) {
    cpu_.reset();
    cpu_.set_pc(BiosResetVector);
    return run(instruction_budget);
}

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
            // A fetched instruction can raise an architected exception (for
            // example SYSCALL, BREAK, or an address fault). The CPU has already
            // moved PC to the exception vector; let the handler execute within
            // the same bounded run. Unsupported opcodes also vector today, but
            // remain a diagnostic stop rather than being mistaken for a valid
            // exception path.
            const bool entered_exception_handler =
                fetched && cpu_.pc() == Cpu::GeneralExceptionVector &&
                error.rfind("Unsupported instruction", 0) != 0;
            if (entered_exception_handler) {
                continue;
            }

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
