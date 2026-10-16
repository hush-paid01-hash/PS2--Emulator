#pragma once

#include "retroshelf/cpu.hpp"
#include "retroshelf/ps2_ee_memory_map.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace retroshelf {

// Bounded interpreter loop for the currently implemented EE instruction subset.
// This is a development runner, not a complete PS2 hardware/BIOS implementation.
class EeExecutionRunner {
public:
    enum class StopReason {
        InstructionLimit,
        CpuExceptionOrUnsupportedInstruction,
        InvalidInstructionFetch
    };

    struct Result {
        std::size_t instructions_executed{0};
        StopReason reason{StopReason::InstructionLimit};
        std::uint64_t final_pc{0};
        std::string message;
    };

    EeExecutionRunner(Cpu& cpu, Ps2EeMemoryMap& memory);
    Result run(std::size_t instruction_budget);

private:
    Cpu& cpu_;
    Ps2EeMemoryMap& memory_;
};

} // namespace retroshelf
