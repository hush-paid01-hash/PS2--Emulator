#include "retroshelf/cpu.hpp"
#include "retroshelf/ee_execution_runner.hpp"
#include "retroshelf/iso9660_disc.hpp"
#include "retroshelf/ps2_disc_execution.hpp"
#include "retroshelf/ps2_ee_memory_map.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
constexpr std::size_t DefaultBudget = 10000;
const char* stop_reason(retroshelf::EeExecutionRunner::StopReason reason) {
    using Reason = retroshelf::EeExecutionRunner::StopReason;
    switch (reason) {
        case Reason::InstructionLimit: return "instruction limit reached";
        case Reason::NoInstructionsRequested: return "zero instruction budget";
        case Reason::CpuExceptionOrUnsupportedInstruction: return "CPU exception or unsupported instruction";
        case Reason::InvalidInstructionFetch: return "invalid instruction fetch";
        case Reason::ExecutionError: return "execution error";
        case Reason::NotStarted: return "not started";
    }
    return "unknown";
}

void print_trace(const retroshelf::EeExecutionRunner::Result& result) {
    std::cout << "Instructions executed: " << result.instructions_executed << "\n"
              << "Final PC: 0x" << std::hex << result.final_pc << std::dec << "\n"
              << "Stop reason: " << stop_reason(result.reason) << "\n"
              << "Detail: " << result.message << "\n"
              << "Trace entries: " << result.trace.size() << "\n";
    for (const auto& entry : result.trace) {
        std::cout << "PC=0x" << std::hex << std::setw(8) << std::setfill('0') << entry.pc
                  << " instruction=0x" << std::setw(8) << entry.instruction
                  << std::dec << " fetched=" << (entry.fetched ? "yes" : "no")
                  << " completed=" << (entry.instruction_completed ? "yes" : "no");
        if (entry.exception_taken)
            std::cout << " exception=" << entry.exception_code
                      << " EPC=0x" << std::hex << entry.exception_epc << std::dec;
        std::cout << "\n";
    }
}

bool parse_budget(const char* text, std::size_t& budget) {
    try {
        const auto parsed = std::stoull(text);
        if (parsed == 0 || parsed > 1000000ULL) return false;
        budget = static_cast<std::size_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

int run_bios(const std::string& path, std::size_t budget) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::cerr << "Cannot open BIOS image: " << path << "\n";
        return 2;
    }
    std::vector<std::uint8_t> image((std::istreambuf_iterator<char>(input)),
                                    std::istreambuf_iterator<char>());
    retroshelf::Ps2EeMemoryMap memory;
    std::string error;
    if (!memory.load_bios_image(image, &error)) {
        std::cerr << "Cannot map BIOS image: " << error << "\n";
        return 2;
    }
    retroshelf::Cpu cpu;
    retroshelf::EeExecutionRunner runner(cpu, memory);
    const auto result = runner.run_from_bios_reset_vector(budget);
    std::cout << "BIOS reset-vector diagnostic (not a complete firmware boot)\n";
    print_trace(result);
    return result.instructions_executed > 0 ? 0 : 3;
}

int run_disc(const std::string& path, std::size_t budget) {
    retroshelf::Iso9660Disc disc;
    std::string error;
    if (!disc.open(path, &error)) {
        std::cerr << "Cannot open PS2 disc image: " << error << "\n";
        return 2;
    }
    retroshelf::Ps2EeMemoryMap memory;
    retroshelf::Cpu cpu;
    const auto result = retroshelf::load_and_execute_ps2_disc(disc, memory, cpu, budget);
    if (!result.image_loaded) {
        std::cerr << "Disc ELF load failed: " << result.error << "\n";
        return 2;
    }
    std::cout << "Disc ELF subset diagnostic (not full PS2 game execution)\n"
              << "Executable: " << result.executable_path << "\n"
              << "ELF entry: 0x" << std::hex << result.entry_point << std::dec << "\n";
    print_trace(result.execution);
    return result.execution.instructions_executed > 0 ? 0 : 3;
}
} // namespace

int main(int argc, char** argv) {
    if (argc >= 3 && (std::string(argv[1]) == "--bios" ||
                      std::string(argv[1]) == "--disc")) {
        std::size_t budget = DefaultBudget;
        if (argc >= 4 && !parse_budget(argv[3], budget)) {
            std::cerr << "Budget must be between 1 and 1000000 instructions.\n";
            return 2;
        }
        return std::string(argv[1]) == "--bios"
            ? run_bios(argv[2], budget) : run_disc(argv[2], budget);
    }
    if (argc == 2 && (std::string(argv[1]) == "--help" ||
                      std::string(argv[1]) == "-h")) {
        std::cout << "Usage:\n"
                  << "  retroshelf-engine --bios BIOS_FILE [instruction-budget]\n"
                  << "  retroshelf-engine --disc GAME.iso [instruction-budget]\n"
                  << "  retroshelf-engine             (small CPU foundation demo)\n";
        return 0;
    }

    retroshelf::Cpu cpu;
    constexpr std::uint32_t load_seven = (0x09U << 26U) | (8U << 16U) | 7U;
    constexpr std::uint32_t load_five = (0x09U << 26U) | (9U << 16U) | 5U;
    constexpr std::uint32_t add = (8U << 21U) | (9U << 16U) | (10U << 11U) | 0x21U;
    if (!cpu.step(load_seven) || !cpu.step(load_five) || !cpu.step(add)) {
        std::cerr << "CPU demo failed to execute an instruction.\n";
        return 1;
    }
    std::cout << "RetroShelf engine foundation\n"
              << "Test calculation: 7 + 5 = " << cpu.reg(10) << "\n"
              << "Instructions tested: 3\n"
              << "Not a playable PS2 emulator yet. Use --help for BIOS/disc diagnostics.\n";
    return cpu.reg(10) == 12 ? 0 : 1;
}
