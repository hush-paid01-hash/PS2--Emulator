#import "RetroShelfBridge.h"
#include "retroshelf/ee_machine.hpp"
#include <string>

extern "C" const char *rs_inspect_ps2_disc(const char *iso_path) {
    static std::string message;
    if (iso_path == nullptr || *iso_path == 0) {
        message = "No disc path was provided.";
        return message.c_str();
    }
    retroshelf::ee::EeMachine machine;
    retroshelf::media::Ps2BootInfo boot;
    std::string error;
    if (!machine.load_ps2_disc(iso_path, &boot, &error)) {
        message = "Disc/ELF load failed: " + error;
        return message.c_str();
    }
    message = "ELF segments loaded into prototype EE RAM. Entry point: 0x";
    constexpr char hex[] = "0123456789ABCDEF";
    const auto entry = static_cast<std::uint32_t>(machine.cpu().pc());
    for (int shift = 28; shift >= 0; shift -= 4)
        message.push_back(hex[(entry >> shift) & 0xFU]);
    message += ". Full PS2 boot, BIOS execution, accurate hardware, and graphics are not implemented.";
    return message.c_str();
}
