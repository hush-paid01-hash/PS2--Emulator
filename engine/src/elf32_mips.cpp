#include "retroshelf/elf32_mips.hpp"

#include <utility>

namespace retroshelf {
namespace {
bool range_ok(std::size_t size, std::uint64_t offset, std::uint64_t length) {
    return offset <= size && length <= static_cast<std::uint64_t>(size) - offset;
}
std::uint16_t u16(const std::vector<std::uint8_t>& b, std::size_t p) {
    return static_cast<std::uint16_t>(b[p] | (static_cast<std::uint16_t>(b[p+1]) << 8U));
}
std::uint32_t u32(const std::vector<std::uint8_t>& b, std::size_t p) {
    return static_cast<std::uint32_t>(b[p]) |
           (static_cast<std::uint32_t>(b[p+1]) << 8U) |
           (static_cast<std::uint32_t>(b[p+2]) << 16U) |
           (static_cast<std::uint32_t>(b[p+3]) << 24U);
}
bool fail(std::string* error, const char* message) {
    if (error) *error = message;
    return false;
}
}

bool parse_elf32_mips(const std::vector<std::uint8_t>& bytes,
                      Elf32MipsImage& image, std::string* error) {
    image = {};
    constexpr std::size_t header_size=52, program_header_size=32;
    if (bytes.size() < header_size) return fail(error,"ELF header is truncated");
    if (bytes[0]!=0x7f || bytes[1]!='E' || bytes[2]!='L' || bytes[3]!='F')
        return fail(error,"Invalid ELF magic");
    if (bytes[4]!=1) return fail(error,"Expected ELFCLASS32");
    if (bytes[5]!=1) return fail(error,"Expected little-endian ELF data");
    if (bytes[6]!=1) return fail(error,"Unsupported ELF identification version");
    if (u16(bytes,16)!=2) return fail(error,"ELF is not an executable");
    if (u16(bytes,18)!=8) return fail(error,"ELF machine is not MIPS");
    if (u32(bytes,20)!=1) return fail(error,"Unsupported ELF header version");
    const auto entry=u32(bytes,24);
    const auto phoff=u32(bytes,28);
    const auto ehsize=u16(bytes,40);
    const auto phentsize=u16(bytes,42);
    const auto phnum=u16(bytes,44);
    if (ehsize < header_size) return fail(error,"Invalid ELF header size");
    if (phnum==0) return fail(error,"ELF has no program headers");
    if (phentsize < program_header_size) return fail(error,"Invalid program-header size");
    const std::uint64_t table_size=static_cast<std::uint64_t>(phentsize)*phnum;
    if (!range_ok(bytes.size(),phoff,table_size))
        return fail(error,"Program-header table is outside the file");

    bool entry_is_executable = false;
    for (std::uint16_t i=0;i<phnum;++i) {
        const auto p=static_cast<std::size_t>(phoff)+static_cast<std::size_t>(i)*phentsize;
        if (u32(bytes,p)!=1) continue; // PT_LOAD only
        const auto offset=u32(bytes,p+4);
        const auto vaddr=u32(bytes,p+8);
        const auto filesz=u32(bytes,p+16);
        const auto memsz=u32(bytes,p+20);
        const auto flags=u32(bytes,p+24);
        if (filesz>memsz) return fail(error,"ELF load segment file size exceeds memory size");
        if (!range_ok(bytes.size(),offset,filesz))
            return fail(error,"ELF load segment extends beyond file");
        const std::uint64_t virtual_end =
            static_cast<std::uint64_t>(vaddr) + static_cast<std::uint64_t>(memsz);
        if (virtual_end > (std::uint64_t{1} << 32U))
            return fail(error,"ELF load segment wraps the 32-bit address space");
        if (memsz==0) continue;
        if ((flags & 1U) != 0U && static_cast<std::uint64_t>(entry) >= vaddr &&
            static_cast<std::uint64_t>(entry) < virtual_end)
            entry_is_executable = true;
        Elf32LoadSegment segment;
        segment.virtual_address=vaddr;
        segment.memory_size=memsz;
        segment.flags=flags;
        segment.file_bytes.assign(bytes.begin()+offset,bytes.begin()+offset+filesz);
        image.load_segments.push_back(std::move(segment));
    }
    if (image.load_segments.empty())
        return fail(error,"ELF has no non-empty PT_LOAD segments");
    if (!entry_is_executable) {
        image = {};
        return fail(error,"ELF entry point is not inside an executable load segment");
    }
    image.entry_point=entry;
    if (error) error->clear();
    return true;
}

} // namespace retroshelf
