#include "retroshelf/elf_ram_loader.hpp"

#include "retroshelf/elf_memory_staging.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
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
    if (!stage_elf32_segments(image, ram_base, ram.size(), staged, entry_offset, error))
        return false;
    if (entry_offset > std::numeric_limits<std::uint32_t>::max() - ram_base)
        return fail(error, "Entry point exceeds the 32-bit address space");
    ram = std::move(staged);
    entry_point = ram_base + static_cast<std::uint32_t>(entry_offset);
    if (error) error->clear();
    return true;
}

bool load_elf32_into_ee_memory(const Elf32MipsImage& image,
                               Ps2EeMemoryMap& memory,
                               std::uint32_t& entry_point,
                               std::string* error) {
    entry_point = 0;
    if (memory.main_ram_size() == 0)
        return fail(error, "EE main RAM is empty");
    if ((image.entry_point & 3U) != 0)
        return fail(error, "ELF entry point is not 4-byte aligned");
    struct ResolvedSegment { std::size_t offset; std::size_t size; const Elf32LoadSegment* segment; };
    std::vector<ResolvedSegment> resolved;
    resolved.reserve(image.load_segments.size());
    bool entry_is_executable = false;

    for (const auto& segment : image.load_segments) {
        if (segment.file_bytes.size() > segment.memory_size)
            return fail(error, "Segment file bytes exceed memory size");
        if (segment.memory_size == 0) continue;
        std::size_t offset = 0;
        if (!memory.resolve_main_ram(segment.virtual_address, 1, 1, offset, error))
            return fail(error, "ELF segment start is outside mapped EE main RAM");
        if (segment.memory_size > memory.main_ram_size() ||
            offset > memory.main_ram_size() - segment.memory_size)
            return fail(error, "ELF segment exceeds mapped EE main RAM");
        const std::uint64_t virtual_end =
            static_cast<std::uint64_t>(segment.virtual_address) + segment.memory_size;
        if (virtual_end > (std::uint64_t{1} << 32U))
            return fail(error, "ELF segment wraps the 32-bit address space");
        for (const auto& prior : resolved) {
            const auto end = offset + segment.memory_size;
            const auto prior_end = prior.offset + prior.size;
            if (offset < prior_end && prior.offset < end)
                return fail(error, "ELF load segments overlap in physical EE RAM");
        }
        resolved.push_back({offset, segment.memory_size, &segment});
        if ((segment.flags & 1U) != 0 &&
            static_cast<std::uint64_t>(image.entry_point) >= segment.virtual_address &&
            static_cast<std::uint64_t>(image.entry_point) < virtual_end) {
            std::size_t entry_offset = 0;
            if (!memory.resolve_main_ram(image.entry_point, 1, 1, entry_offset, error))
                return fail(error, "ELF entry point is outside mapped EE main RAM");
            entry_is_executable = true;
        }
    }
    if (!entry_is_executable)
        return fail(error, "ELF entry point is not inside an executable load segment");
    auto staged = memory.main_ram();
    for (const auto& item : resolved) {
        std::fill(staged.begin() + static_cast<std::ptrdiff_t>(item.offset),
                  staged.begin() + static_cast<std::ptrdiff_t>(item.offset + item.size), 0);
        std::copy(item.segment->file_bytes.begin(), item.segment->file_bytes.end(),
                  staged.begin() + static_cast<std::ptrdiff_t>(item.offset));
    }
    memory.main_ram() = std::move(staged);
    entry_point = image.entry_point;
    if (error) error->clear();
    return true;
}
} // namespace retroshelf
