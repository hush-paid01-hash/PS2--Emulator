#pragma once

#include "retroshelf/iso9660_disc.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf {

// Resolves SYSTEM.CNF from a disc image and reads the referenced executable.
// This only prepares bytes; it does not map PS2 memory or execute the ELF.
bool load_ps2_boot_executable(const Iso9660Disc& disc,
                              std::string& executable_path,
                              std::vector<std::uint8_t>& executable,
                              std::string* error = nullptr);

} // namespace retroshelf
