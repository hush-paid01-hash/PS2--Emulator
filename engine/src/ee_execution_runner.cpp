#include "retroshelf/ee_execution_runner.hpp"
#include "retroshelf/system_bus.hpp"
#include "retroshelf/ee_interrupt_controller.hpp"

namespace retroshelf {

EeExecutionRunner::EeExecutionRunner(Cpu& cpu, Ps2EeMemoryMap& memory)
    : cpu_(cpu), memory_(memory) {}

EeExecutionRunner::EeExecutionRunner(Cpu& cpu, Ps2EeMemoryMap& memory, SystemBus& bus)
    : cpu_(cpu), memory_(memory), bus_(&bus) {}

EeExecutionRunner::EeExecutionRunner(Cpu& cpu, Ps2EeMemoryMap& memory,
                                     SystemBus& bus, EeInterruptController& intc)
    : cpu_(cpu), memory_(memory), bus_(&bus), intc_(&intc) {}

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
        const bool delay_slot = cpu_.in_delay_slot();
        std::uint64_t raw_instruction = 0;
        std::string fetch_error;
        const bool fetched = instruction_pc <= 0xFFFFFFFFULL &&
            memory_.read(static_cast<std::uint32_t>(instruction_pc), 4, raw_instruction, &fetch_error);

        std::string error;
        cpu_.set_ee_interrupt_pending(intc_ != nullptr && intc_->active_pending() != 0U);
        const bool instruction_completed = bus_ != nullptr
            ? cpu_.step_from_ee_bus(memory_, *bus_, &error)
            : cpu_.step_from_ee_memory(memory_, &error);
        const bool exception_taken = !instruction_completed &&
            instruction_pc != Cpu::GeneralExceptionVector &&
            cpu_.pc() == Cpu::GeneralExceptionVector &&
            (cpu_.cop0_status() & Cpu::StatusExl) != 0U;
        const std::uint32_t cause = cpu_.cop0_cause();
        if (result.trace.size() < TraceCapacity) {
            TraceEntry entry;
            entry.pc = instruction_pc;
            entry.instruction = static_cast<std::uint32_t>(raw_instruction);
            entry.fetched = fetched;
            entry.delay_slot = delay_slot;
            entry.instruction_completed = instruction_completed;
            entry.exception_taken = exception_taken;
            entry.exception_code = exception_taken ? ((cause >> 2U) & 0x1FU) : 0U;
            entry.exception_in_delay_slot = exception_taken && ((cause & Cpu::CauseBd) != 0U);
            entry.exception_epc = exception_taken ? cpu_.cop0_epc() : 0U;
            entry.bad_vaddr = exception_taken ? cpu_.cop0_bad_vaddr() : 0U;
            entry.next_pc = cpu_.next_pc();
            entry.cop0_status = cpu_.cop0_status();
            result.trace.push_back(entry);
        }

        if (!instruction_completed) {
            // A fetched instruction can raise an architected exception (for
            // example SYSCALL, BREAK, or an address fault). The CPU has already
            // moved PC to the exception vector; let the handler execute within
            // the same bounded run. Unsupported opcodes also vector today, but
            // remain a diagnostic stop rather than being mistaken for a valid
            // exception path.
            const bool entered_exception_handler =
                instruction_pc != Cpu::GeneralExceptionVector &&
                cpu_.pc() == Cpu::GeneralExceptionVector &&
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
