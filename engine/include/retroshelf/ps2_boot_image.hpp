#pragma once

#include "retroshelf/elf32_mips.hpp"
#include "retroshelf/iso9660_disc.hpp"
#include "retroshelf/ps2_ee_memory_map.hpp"

#include <string>

namespace retroshelf {

// Resolves SYSTEM.CNF, reads its executable, and validates its ELF32 MIPS
// load segments. This returns a load plan; it does not map RAM or execute code.
bool load_ps2_boot_image(const Iso9660Disc& disc,
                         std::string& executable_path,
                         Elf32MipsImage& image,
                         std::string* error = nullptr);

// Resolves SYSTEM.CNF, parses its ELF, and loads PT_LOAD segments into EE main
// RAM. On failure, the memory map is unchanged and output values are cleared.
bool boot_ps2_disc_into_ee_memory(const Iso9660Disc& disc,
                                  Ps2EeMemoryMap& memory,
                                  std::string& executable_path,
                                  std::uint32_t& entry_point,
                                  std::string* error = nullptr);

} // namespace retroshelf
