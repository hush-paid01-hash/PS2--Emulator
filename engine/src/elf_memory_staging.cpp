#include "retroshelf/elf_memory_staging.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <utility>

namespace retroshelf {
namespace {
bool fail(std::string* error, const char* message) {
    if (error) *error = message;
    return false;
}
}

bool stage_elf32_segments(const Elf32MipsImage& image,
                          std::uint32_t window_base,
                          std::size_t window_size,
                          std::vector<std::uint8_t>& memory,
                          std::size_t& entry_offset,
                          std::string* error) {
    memory.clear();
    entry_offset = 0;
    if (window_size == 0) return fail(error, "Memory window is empty");
    if ((image.entry_point & 3U) != 0)
        return fail(error, "ELF entry point is not 4-byte aligned");
    const std::uint64_t window_end =
        static_cast<std::uint64_t>(window_base) + window_size;
    if (window_end > (std::uint64_t{1} << 32U))
        return fail(error, "Memory window exceeds the 32-bit address space");

    std::vector<std::uint8_t> staged(window_size, 0);
    bool entry_is_executable = false;
    for (const auto& segment : image.load_segments) {
        if (segment.file_bytes.size() > segment.memory_size)
            return fail(error, "Segment file bytes exceed memory size");
        const std::uint64_t start = segment.virtual_address;
        const std::uint64_t end = start + segment.memory_size;
        if (start < window_base || end > window_end || end < start)
            return fail(error, "ELF segment is outside the supplied memory window");
        const auto offset = static_cast<std::size_t>(start - window_base);
        const auto length = static_cast<std::size_t>(segment.memory_size);
        for (const auto& prior : image.load_segments) {
            if (&prior == &segment) break;
            const std::uint64_t prior_start = prior.virtual_address;
            const std::uint64_t prior_end = prior_start + prior.memory_size;
            if (start < prior_end && prior_start < end)
                return fail(error, "ELF load segments overlap");
        }
        std::copy(segment.file_bytes.begin(), segment.file_bytes.end(),
                  staged.begin() + static_cast<std::ptrdiff_t>(offset));
        // Remaining bytes in this PT_LOAD range stay zero (ELF BSS).
        (void)length;
        if ((segment.flags & 1U) != 0 &&
            static_cast<std::uint64_t>(image.entry_point) >= start &&
            static_cast<std::uint64_t>(image.entry_point) < end) {
            entry_is_executable = true;
            entry_offset = static_cast<std::size_t>(image.entry_point - window_base);
        }
    }
    if (!entry_is_executable)
        return fail(error, "ELF entry point is not inside an executable load segment");
    memory = std::move(staged);
    if (error) error->clear();
    return true;
}

} // namespace retroshelf
