#include "retroshelf/ee_execution_runner.hpp"

#include <cassert>
#include <cstdint>
#include <string>

int main() {
    retroshelf::Ps2EeMemoryMap memory(64U * 1024U);
    retroshelf::Cpu cpu;
    std::string error;
    constexpr std::uint32_t start = 0x80001000U;

    // ADDIU t0, zero, 7; ADDIU t1, zero, 5; ADDU t2, t0, t1.
    assert(memory.write(start, 4, (0x09U << 26U) | (8U << 16U) | 7U, &error));
    assert(memory.write(start + 4U, 4, (0x09U << 26U) | (9U << 16U) | 5U, &error));
    assert(memory.write(start + 8U, 4, (8U << 21U) | (9U << 16U) | (10U << 11U) | 0x21U, &error));
    cpu.set_pc(start);

    retroshelf::EeExecutionRunner runner(cpu, memory);
    const auto first = runner.run(2);
    assert(first.instructions_executed == 2);
    assert(first.reason == retroshelf::EeExecutionRunner::StopReason::InstructionLimit);
    assert(cpu.reg(8) == 7 && cpu.reg(9) == 5);
    assert(cpu.reg(10) == 0);
    assert(cpu.pc() == start + 8U);
    assert(first.trace.size() == 2);
    assert(first.trace[0].pc == start && first.trace[0].fetched);
    assert(first.trace[0].instruction == ((0x09U << 26U) | (8U << 16U) | 7U));
    assert(first.trace[1].pc == start + 4U && first.trace[1].fetched);

    const auto second = runner.run(1);
    assert(second.instructions_executed == 1);
    assert(cpu.reg(10) == 12);
    assert(second.final_pc == start + 12U);
    assert(second.trace.size() == 1 && second.trace[0].pc == start + 8U);

    const auto zero = runner.run(0);
    assert(zero.instructions_executed == 0);
    assert(zero.reason == retroshelf::EeExecutionRunner::StopReason::NoInstructionsRequested);
    assert(!zero.message.empty());
    assert(zero.final_pc == cpu.pc());
    assert(zero.trace.empty());

    // The trace captures the exact unsupported instruction before the CPU traps.
    constexpr std::uint32_t unsupported = 0x3EU << 26U;
    assert(memory.write(start + 12U, 4, unsupported, &error));
    const auto stopped = runner.run(1);
    assert(stopped.instructions_executed == 0);
    assert(stopped.reason == retroshelf::EeExecutionRunner::StopReason::CpuExceptionOrUnsupportedInstruction);
    assert(stopped.final_pc == retroshelf::Cpu::GeneralExceptionVector);
    assert(!stopped.message.empty());
    assert(stopped.trace.size() == 1);
    assert(stopped.trace[0].pc == start + 12U);
    assert(stopped.trace[0].instruction == unsupported);
    assert(stopped.trace[0].fetched);

    constexpr std::uint32_t exception_vector = 0x80000180U;
    assert(memory.write(exception_vector, 4, (0x09U << 26U) | (11U << 16U) | 9U, &error));
    const auto handler = runner.run(1);
    assert(handler.instructions_executed == 1);
    assert(handler.reason == retroshelf::EeExecutionRunner::StopReason::InstructionLimit);
    assert(cpu.reg(11) == 9);
    assert(handler.final_pc == exception_vector + 4U);

    // Start a fresh exception context: the prior unsupported-instruction test
    // intentionally left Status.EXL set while running its handler.
    cpu.reset();
    cpu.set_pc(0x30000000U);
    const auto fetch_fault = runner.run(1);
    assert(fetch_fault.instructions_executed == 0);
    assert(fetch_fault.reason == retroshelf::EeExecutionRunner::StopReason::InvalidInstructionFetch);
    assert(fetch_fault.final_pc == retroshelf::Cpu::GeneralExceptionVector);
    assert(cpu.cop0_epc() == 0x30000000U);
    assert(cpu.cop0_bad_vaddr() == 0x30000000U);
    assert(fetch_fault.trace.size() == 1);
    assert(fetch_fault.trace[0].pc == 0x30000000U);
    assert(!fetch_fault.trace[0].fetched);
    const auto fetch_handler = runner.run(1);
    assert(fetch_handler.instructions_executed == 1);
    assert(cpu.reg(11) == 9);
    assert(fetch_handler.final_pc == exception_vector + 4U);

    // Trace storage is bounded even when the execution budget is large.
    // Replace the unsupported instruction with a NOP so the run can continue.
    assert(memory.write(start + 12U, 4, 0U, &error));
    cpu.set_pc(start);
    const auto bounded = runner.run(retroshelf::EeExecutionRunner::TraceCapacity + 1U);
    assert(bounded.trace.size() == retroshelf::EeExecutionRunner::TraceCapacity);
    return 0;
}
