#pragma once

#include <string>

namespace retroshelf {

// Extract the executable path from a PS2 SYSTEM.CNF text file.
// BOOT2 takes precedence over BOOT when both entries are present.
bool parse_system_cnf(const std::string& contents, std::string& executable_path,
                      std::string* error = nullptr);

} // namespace retroshelf
