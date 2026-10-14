#include "retroshelf/ps2_elf.hpp"
#include "retroshelf/disc_image.hpp"

#include <algorithm>
#include <limits>

namespace retroshelf::media {
namespace {
constexpr std::size_t ElfHeaderSize = 52;
constexpr std::size_t ProgramHeaderSize = 32;
constexpr std::uint32_t PtLoad = 1;
constexpr std::uint16_t MachineMips = 8;
std::uint16_t le16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0] | (static_cast<std::uint16_t>(p[1]) << 8U));
}
std::uint32_t le32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8U)
        | (static_cast<std::uint32_t>(p[2]) << 16U) | (static_cast<std::uint32_t>(p[3]) << 24U);
}
Ps2ElfImage invalid(const char* message) {
    Ps2ElfImage result; result.error = message; return result;
}
}

Ps2ElfImage parse_ps2_elf(const std::vector<std::uint8_t>& image) {
    if (image.size() < ElfHeaderSize) return invalid("ELF header is truncated");
    if (image[0] != 0x7f || image[1] != 'E' || image[2] != 'L' || image[3] != 'F')
        return invalid("ELF magic is missing");
    if (image[4] != 1 || image[5] != 1 || image[6] != 1)
        return invalid("Only ELF32 little-endian version 1 executables are supported");
    if (le16(image.data() + 18) != MachineMips)
        return invalid("ELF machine type is not MIPS");
    if (le16(image.data() + 16) != 2)
        return invalid("ELF file is not an executable");
    if (le16(image.data() + 40) < ElfHeaderSize)
        return invalid("ELF header size is invalid");

    const auto entry = le32(image.data() + 24);
    const auto phoff = le32(image.data() + 28);
    const auto phentsize = le16(image.data() + 42);
    const auto phnum = le16(image.data() + 44);
    if (phnum == 0 || phentsize < ProgramHeaderSize)
        return invalid("ELF program header table is empty or malformed");
    const std::uint64_t table_end = static_cast<std::uint64_t>(phoff)
        + static_cast<std::uint64_t>(phentsize) * phnum;
    if (table_end > image.size()) return invalid("ELF program header table exceeds file size");

    Ps2ElfImage result;
    result.entry_point = entry;
    for (std::uint16_t i = 0; i < phnum; ++i) {
        const auto* p = image.data() + phoff + static_cast<std::size_t>(i) * phentsize;
        if (le32(p) != PtLoad) continue;
        const auto offset = le32(p + 4);
        const auto vaddr = le32(p + 8);
        const auto filesz = le32(p + 16);
        const auto memsz = le32(p + 20);
        const auto flags = le32(p + 24);
        if (filesz > memsz) return invalid("ELF load segment file size exceeds memory size");
        const std::uint64_t file_end = static_cast<std::uint64_t>(offset) + filesz;
        const std::uint64_t mem_end = static_cast<std::uint64_t>(vaddr) + memsz;
        if (file_end > image.size()) return invalid("ELF load segment exceeds file size");
        if (mem_end > (static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) + 1ULL))
            return invalid("ELF load segment address overflows 32-bit address space");
        if (memsz == 0) continue;
        ElfLoadSegment segment;
        segment.virtual_address = vaddr;
        segment.file_size = filesz;
        segment.memory_size = memsz;
        segment.flags = flags;
        segment.bytes.resize(memsz, 0);
        std::copy_n(image.begin() + offset, filesz, segment.bytes.begin());
        result.segments.push_back(std::move(segment));
    }
    if (result.segments.empty()) return invalid("ELF contains no loadable segments");
    const bool entry_mapped = std::any_of(result.segments.begin(), result.segments.end(),
        [entry](const ElfLoadSegment& s) {
            return entry >= s.virtual_address
                && static_cast<std::uint64_t>(entry) < static_cast<std::uint64_t>(s.virtual_address) + s.memory_size;
        });
    if (!entry_mapped) return invalid("ELF entry point is outside all loadable segments");
    result.valid = true;
    return result;
}

Ps2ElfImage load_ps2_boot_elf_from_disc(const std::string& iso_path) {
    const auto boot = find_ps2_boot_executable(iso_path);
    if (!boot.found) return invalid(boot.error.c_str());
    std::vector<std::uint8_t> bytes;
    std::string error;
    if (!read_iso_file(iso_path, boot.executable_lba, boot.executable_size, bytes, &error))
        return invalid(error.c_str());
    return parse_ps2_elf(bytes);
}
} // namespace retroshelf::media
