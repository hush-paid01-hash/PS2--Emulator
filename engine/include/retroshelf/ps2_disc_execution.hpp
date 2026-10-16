#pragma once

#include "retroshelf/ee_execution_runner.hpp"
#include "retroshelf/iso9660_disc.hpp"
#include "retroshelf/ps2_boot_image.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace retroshelf {

// Loads a disc's SYSTEM.CNF ELF into EE RAM, sets the CPU entry PC, then
// executes at most instruction_budget instructions using the current subset CPU.
// This is a development execution path, not BIOS boot or full PS2 compatibility.
struct Ps2DiscExecutionResult {
    bool image_loaded{false};
    std::string executable_path;
    std::uint32_t entry_point{0};
    EeExecutionRunner::Result execution;
    std::string error;
};

Ps2DiscExecutionResult load_and_execute_ps2_disc(const Iso9660Disc& disc,
                                                  Ps2EeMemoryMap& memory,
                                                  Cpu& cpu,
                                                  std::size_t instruction_budget);

} // namespace retroshelf
