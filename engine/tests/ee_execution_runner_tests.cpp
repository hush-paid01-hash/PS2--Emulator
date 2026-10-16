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

    const auto second = runner.run(1);
    assert(second.instructions_executed == 1);
    assert(cpu.reg(10) == 12);
    assert(second.final_pc == start + 12U);

    const auto zero = runner.run(0);
    assert(zero.instructions_executed == 0);
    assert(zero.final_pc == cpu.pc());

    // An unsupported opcode must stop the runner and preserve the CPU exception.
    assert(memory.write(start + 12U, 4, 0x3EU << 26U, &error));
    const auto stopped = runner.run(1);
    assert(stopped.instructions_executed == 0);
    assert(stopped.reason == retroshelf::EeExecutionRunner::StopReason::CpuExceptionOrUnsupportedInstruction);
    assert(stopped.final_pc == retroshelf::Cpu::GeneralExceptionVector);
    assert(!stopped.message.empty());
    return 0;
}
