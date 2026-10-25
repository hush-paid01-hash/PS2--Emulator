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
        NoInstructionsRequested,
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
    static constexpr std::uint32_t BiosResetVector = 0xBFC00000U;

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

    // Reset the interpreter state and begin fetching at the EE BIOS reset vector.
    // This is a convenient entry point for the current subset CPU, not a claim
    // that the full PS2 reset state or BIOS boot process is implemented.
    Result run_from_bios_reset_vector(std::size_t instruction_budget);

private:
    Cpu& cpu_;
    Ps2EeMemoryMap& memory_;
};

} // namespace retroshelf
