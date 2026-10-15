#pragma once

#include "retroshelf/elf32_mips.hpp"
#include "retroshelf/iso9660_disc.hpp"

#include <string>

namespace retroshelf {

// Resolves SYSTEM.CNF, reads its executable, and validates its ELF32 MIPS
// load segments. This returns a load plan; it does not map RAM or execute code.
bool load_ps2_boot_image(const Iso9660Disc& disc,
                         std::string& executable_path,
                         Elf32MipsImage& image,
                         std::string* error = nullptr);

} // namespace retroshelf
