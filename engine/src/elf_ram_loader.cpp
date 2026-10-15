#include "retroshelf/elf_ram_loader.hpp"

#include "retroshelf/elf_memory_staging.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace retroshelf {
namespace {
bool fail(std::string* error, const char* message) {
    if (error) *error = message;
    return false;
}
}

bool load_elf32_into_ram(const Elf32MipsImage& image,
                         std::uint32_t ram_base,
                         std::vector<std::uint8_t>& ram,
                         std::uint32_t& entry_point,
                         std::string* error) {
    entry_point = 0;
    if (ram.empty()) return fail(error, "RAM buffer is empty");

    std::vector<std::uint8_t> staged;
    std::size_t entry_offset = 0;
    if (!stage_elf32_segments(image, ram_base, ram.size(), staged,
                              entry_offset, error)) {
        return false;
    }
    if (entry_offset > UINT32_MAX - ram_base)
        return fail(error, "Entry point exceeds the 32-bit address space");

    // Commit only after every segment and the entry point have validated.
    ram = std::move(staged);
    entry_point = ram_base + static_cast<std::uint32_t>(entry_offset);
    if (error) error->clear();
    return true;
}

} // namespace retroshelf
