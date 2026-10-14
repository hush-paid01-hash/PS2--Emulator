#import "RetroShelfBridge.h"
#include "retroshelf/ee_machine.hpp"
#include <string>

namespace { retroshelf::ee::EeMachine diagnostic_machine; bool diagnostic_loaded = false; }

extern "C" const char *rs_inspect_ps2_disc(const char *iso_path) {
    static std::string message;
    if (iso_path == nullptr || *iso_path == 0) {
        message = "No disc path was provided.";
        return message.c_str();
    }
    retroshelf::media::Ps2BootInfo boot;
    std::string error;
    if (!diagnostic_machine.load_ps2_disc(iso_path, &boot, &error)) {
        diagnostic_loaded = false;
        message = "Disc/ELF load failed: " + error;
        return message.c_str();
    }
    diagnostic_loaded = true;
    message = "ELF segments loaded into prototype EE RAM. Entry point: 0x";
    constexpr char hex[] = "0123456789ABCDEF";
    const auto entry = static_cast<std::uint32_t>(diagnostic_machine.cpu().pc());
    for (int shift = 28; shift >= 0; shift -= 4)
        message.push_back(hex[(entry >> shift) & 0xFU]);
    message += ". Full PS2 boot, BIOS execution, accurate hardware, and graphics are not implemented.";
    return message.c_str();
}

extern "C" const char *rs_run_ee_diagnostic(unsigned long long instruction_budget) {
    static std::string message;
    if (!diagnostic_loaded) {
        message = "Choose a valid PS2 ISO first; no executable is loaded.";
        return message.c_str();
    }
    std::string error;
    const auto executed = diagnostic_machine.run(static_cast<std::size_t>(instruction_budget), &error);
    message = "EE diagnostic executed " + std::to_string(executed) + " instruction(s); PC=0x";
    constexpr char hex[] = "0123456789ABCDEF";
    const auto pc = static_cast<std::uint32_t>(diagnostic_machine.cpu().pc());
    for (int shift = 28; shift >= 0; shift -= 4)
        message.push_back(hex[(pc >> shift) & 0xFU]);
    if (!error.empty()) message += ". Stopped: " + error;
    else message += ". Instruction budget reached. This is a CPU diagnostic, not gameplay.";
    return message.c_str();
}
