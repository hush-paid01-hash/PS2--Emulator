#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace retroshelf {

struct BiosImageInfo {
    std::uint64_t size_bytes{0};
    bool plausible_size{false};
};

// Performs conservative file/size checks only. It does not authenticate a BIOS,
// prove compatibility, or load/execute firmware.
class BiosImageValidator {
public:
    static bool inspect(const std::filesystem::path& path, BiosImageInfo& info,
                        std::string* error = nullptr);
};

} // namespace retroshelf
