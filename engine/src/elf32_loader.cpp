#include "retroshelf/elf32_loader.hpp"

#include <algorithm>
#include <limits>

namespace retroshelf {
namespace {
bool range_ok(std::size_t offset, std::size_t length, std::size_t total) {
    return offset <= total && length <= total - offset;
}
std::uint16_t u16(const std::vector<std::uint8_t>& b, std::size_t p) {
    return static_cast<std::uint16_t>(b[p]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(b[p + 1]) << 8U);
}
std::uint32_t u32(const std::vector<std::uint8_t>& b, std::size_t p) {
    return static_cast<std::uint32_t>(b[p]) |
           (static_cast<std::uint32_t>(b[p + 1]) << 8U) |
           (static_cast<std::uint32_t>(b[p + 2]) << 16U) |
           (static_cast<std::uint32_t>(b[p + 3]) << 24U);
}
ElfLoadResult fail(const char* why) { return {false, 0, 0, why}; }
}

ElfLoadResult Elf32Loader::load_mips_executable(
    const std::vector<std::uint8_t>& image,
    std::vector<std::uint8_t>& ee_ram) {
    constexpr std::size_t HeaderSize = 52;
    constexpr std::size_t ProgramHeaderSize = 32;
    constexpr std::uint32_t PtLoad = 1;

    if (image.size() < HeaderSize || image[0] != 0x7f ||
        image[1] != 'E' || image[2] != 'L' || image[3] != 'F')
        return fail("not an ELF image or truncated ELF header");
    if (image[4] != 1 || image[5] != 1 || image[6] != 1)
        return fail("only ELF32 little-endian version 1 is supported");
    if (u16(image, 18) != 8)
        return fail("ELF machine is not MIPS");
    if (u32(image, 20) != 1)
        return fail("unsupported ELF version");
    const std::uint32_t entry = u32(image, 24);
    const std::uint32_t phoff = u32(image, 28);
    const std::uint16_t ehsize = u16(image, 40);
    const std::uint16_t phentsize = u16(image, 42);
    const std::uint16_t phnum = u16(image, 44);
    if (ehsize < HeaderSize || phentsize < ProgramHeaderSize)
        return fail("invalid ELF header sizes");
    const std::size_t table_size = static_cast<std::size_t>(phentsize) * phnum;
    if (!range_ok(phoff, table_size, image.size()))
        return fail("program header table exceeds image bounds");

    // Validate all loadable segments before changing RAM, so malformed images
    // cannot leave a partially loaded executable behind.
    struct Segment { std::size_t file_off, address, file_size, mem_size; };
    std::vector<Segment> segments;
    for (std::uint16_t i = 0; i < phnum; ++i) {
        const std::size_t p = static_cast<std::size_t>(phoff) +
                              static_cast<std::size_t>(i) * phentsize;
        if (u32(image, p) != PtLoad) continue;
        const std::uint32_t offset = u32(image, p + 4);
        const std::uint32_t vaddr = u32(image, p + 8);
        const std::uint32_t paddr = u32(image, p + 12);
        const std::uint32_t filesz = u32(image, p + 16);
        const std::uint32_t memsz = u32(image, p + 20);
        const std::uint32_t address = paddr != 0 ? paddr : vaddr;
        if (filesz > memsz)
            return fail("ELF segment file size exceeds memory size");
        if (!range_ok(offset, filesz, image.size()))
            return fail("ELF segment exceeds image bounds");
        if (!range_ok(address, memsz, ee_ram.size()))
            return fail("ELF segment address exceeds supplied EE RAM mapping");
        segments.push_back({offset, address, filesz, memsz});
    }
    if (segments.empty()) return fail("ELF contains no loadable segments");
    if (entry >= ee_ram.size()) return fail("ELF entry point exceeds supplied EE RAM mapping");

    for (const auto& s : segments) {
        auto begin = ee_ram.begin() + static_cast<std::ptrdiff_t>(s.address);
        std::copy_n(image.begin() + static_cast<std::ptrdiff_t>(s.file_off),
                    s.file_size, begin);
        std::fill(begin + static_cast<std::ptrdiff_t>(s.file_size),
                  begin + static_cast<std::ptrdiff_t>(s.mem_size), 0);
    }
    return {true, entry, segments.size(), {}};
}
} // namespace retroshelf
