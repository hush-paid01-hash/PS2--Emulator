#include "retroshelf/bios_image_validator.hpp"

#include <fstream>

namespace retroshelf {

bool BiosImageValidator::inspect(const std::filesystem::path& path,
                                 BiosImageInfo& info,
                                 std::string* error) {
    info = {};
    auto fail = [&](const char* message) {
        if (error) *error = message;
        return false;
    };
    if (path.empty()) return fail("No BIOS file was selected.");

    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec) || ec)
        return fail("The selected BIOS path is not a regular file.");

    const auto size = std::filesystem::file_size(path, ec);
    if (ec) return fail("Could not read the BIOS file size.");
    info.size_bytes = size;
    constexpr std::uint64_t min_size = 1ULL * 1024 * 1024;
    constexpr std::uint64_t max_size = 16ULL * 1024 * 1024;
    info.plausible_size = size >= min_size && size <= max_size;

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        info = {};
        return fail("The BIOS file could not be opened for reading.");
    }
    char byte = 0;
    input.read(&byte, 1);
    if (input.gcount() != 1) {
        info = {};
        return fail("The BIOS file could not be read.");
    }
    if (error) error->clear();
    // A file can be readable but still have an implausible size. Keep these
    // results separate so callers can show a warning instead of claiming that
    // this heuristic proves authenticity or compatibility.
    return true;
}

} // namespace retroshelf
