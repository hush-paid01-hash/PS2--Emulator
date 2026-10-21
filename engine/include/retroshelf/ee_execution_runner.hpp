#pragma once

#include "retroshelf/cpu.hpp"
#include "retroshelf/ps2_ee_memory_map.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf {

// Bounded interpreter loop for the currently implemented EE instruction subset.
// This is a development runner, not a complete PS2 hardware/BIOS implementation.
class EeExecutionRunner {
public:
    enum class StopReason {
        NotStarted,
        InstructionLimit,
        CpuExceptionOrUnsupportedInstruction,
        InvalidInstructionFetch,
        ExecutionError
    };

    struct TraceEntry {
        std::uint64_t pc{0};
        std::uint32_t instruction{0};
        bool fetched{false};
    };

    static constexpr std::size_t TraceCapacity = 256;

    struct Result {
        std::size_t instructions_executed{0};
        StopReason reason{StopReason::NotStarted};
        std::uint64_t final_pc{0};
        std::string message;
        // First TraceCapacity attempted instructions in this run. Keeping the
        // trace bounded makes it safe to use while diagnosing a BIOS boot loop.
        std::vector<TraceEntry> trace;
    };

    EeExecutionRunner(Cpu& cpu, Ps2EeMemoryMap& memory);
    Result run(std::size_t instruction_budget);

private:
    Cpu& cpu_;
    Ps2EeMemoryMap& memory_;
};

} // namespace retroshelf
