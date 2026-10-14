#import "RetroShelfBridge.h"
#include "retroshelf/ps2_elf.hpp"
#include <string>

extern "C" const char *rs_inspect_ps2_disc(const char *iso_path) {
    static std::string message;
    if (iso_path == nullptr || *iso_path == 0) {
        message = "No disc path was provided.";
        return message.c_str();
    }
    const auto image = retroshelf::media::load_ps2_boot_elf_from_disc(iso_path);
    if (!image.valid) {
        message = "Disc/boot executable check failed: " + image.error;
        return message.c_str();
    }
    message = "PS2 boot ELF recognized. Entry point: 0x";
    constexpr char hex[] = "0123456789ABCDEF";
    for (int shift = 28; shift >= 0; shift -= 4)
        message.push_back(hex[(image.entry_point >> shift) & 0xFU]);
    message += ". Executable parsed; CPU boot and graphics are not implemented.";
    return message.c_str();
}
