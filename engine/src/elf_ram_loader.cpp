#include "retroshelf/elf_ram_loader.hpp"

#include "retroshelf/elf_memory_staging.hpp"

#include <cstdint>
#include <utility>
#include <cstddef>
#include <cstdint>
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
                         std::uint32_t ram_address_base,
                         SystemBus& bus,
                         std::uint32_t& entry_point,
                         std::string* error) {
    entry_point = 0;
    if (bus.ram_size() == 0)
        return fail(error, "EE RAM is empty");

    std::vector<std::uint8_t> validation_window;
    std::size_t entry_offset = 0;
    if (!stage_elf32_segments(image, ram_address_base, bus.ram_size(),
                              validation_window, entry_offset, error)) {
        return false;
    }

    // All segments are validated before RAM is modified. Preserve unrelated
    // RAM bytes and write only each segment's memory range.
    for (const auto& segment : image.load_segments) {
        const auto address64 =
            static_cast<std::uint64_t>(segment.virtual_address) - ram_address_base;
        if (address64 > UINT32_MAX)
            return fail(error, "ELF segment address cannot be represented by the bus");
        const auto address = static_cast<std::uint32_t>(address64);
        if (!bus.write(address, segment.file_bytes, error)) return false;
        const auto bss_size =
            static_cast<std::size_t>(segment.memory_size) - segment.file_bytes.size();
        if (bss_size != 0) {
            std::vector<std::uint8_t> zeros(bss_size, 0);
            const auto bss_address64 = address64 + segment.file_bytes.size();
            if (bss_address64 > UINT32_MAX)
                return fail(error, "ELF BSS address cannot be represented by the bus");
            if (!bus.write(static_cast<std::uint32_t>(bss_address64), zeros, error))
                return false;
        }
    }

    entry_point = image.entry_point;
    if (error) error->clear();
    return true;
}

} // namespace retroshelf
