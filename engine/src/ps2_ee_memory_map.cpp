#include "retroshelf/ps2_ee_memory_map.hpp"

#include <utility>

namespace retroshelf {

Ps2EeMemoryMap::Ps2EeMemoryMap() : ram_(MainRamSize, 0) {}
Ps2EeMemoryMap::Ps2EeMemoryMap(std::size_t main_ram_size) : ram_(main_ram_size, 0) {}
std::size_t Ps2EeMemoryMap::main_ram_size() const { return ram_.size(); }
std::vector<std::uint8_t>& Ps2EeMemoryMap::main_ram() { return ram_; }
const std::vector<std::uint8_t>& Ps2EeMemoryMap::main_ram() const { return ram_; }

bool Ps2EeMemoryMap::resolve_main_ram(std::uint32_t address, std::size_t width,
                                      std::size_t alignment, std::size_t& offset,
                                      std::string* error) const {
    const auto fail = [error](const char* message) {
        if (error) *error = message;
        return false;
    };
    if (width != 1 && width != 2 && width != 4 && width != 8 && width != 16)
        return fail("Unsupported memory access width");
    if (alignment == 0 || (alignment & (alignment - 1U)) != 0)
        return fail("Alignment must be a nonzero power of two");

    std::uint32_t physical = address;
    // KSEG0/KSEG1 are direct-mapped cached/uncached aliases in the 32-bit EE view.
    if ((address >= Kseg0Base && address < 0xC0000000U))
        physical = address & SegmentMask;

    if (physical >= MainRamMirrorBase &&
        physical < MainRamMirrorBase + MainRamSize)
        physical -= MainRamMirrorBase;

    if (physical >= MainRamBase && physical < MainRamSize) {
        offset = static_cast<std::size_t>(physical);
    } else {
        return fail("Address is not in a supported EE main-RAM region");
    }
    if ((address & static_cast<std::uint32_t>(alignment - 1U)) != 0U)
        return fail("Misaligned memory access");
    if (width > ram_.size() || offset > ram_.size() - width)
        return fail("Memory access exceeds configured main RAM");
    if (error) error->clear();
    return true;
}

bool Ps2EeMemoryMap::read(std::uint32_t address, std::size_t width,
                          std::uint64_t& value, std::string* error) const {
    std::size_t offset = 0;
    if (!resolve_main_ram(address, width, width, offset, error)) return false;
    value = 0;
    for (std::size_t i = 0; i < width; ++i)
        value |= static_cast<std::uint64_t>(ram_[offset + i]) << (8U * i);
    if (error) error->clear();
    return true;
}

bool Ps2EeMemoryMap::write(std::uint32_t address, std::size_t width,
                           std::uint64_t value, std::string* error) {
    std::size_t offset = 0;
    if (!resolve_main_ram(address, width, width, offset, error)) return false;
    for (std::size_t i = 0; i < width; ++i)
        ram_[offset + i] = static_cast<std::uint8_t>((value >> (8U * i)) & 0xFFU);
    if (error) error->clear();
    return true;
}

bool Ps2EeMemoryMap::read128(std::uint32_t address, std::uint64_t& low,
                             std::uint64_t& high, std::string* error) const {
    std::size_t offset = 0;
    if (!resolve_main_ram(address, 16, 16, offset, error)) return false;
    low = 0;
    high = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        low |= static_cast<std::uint64_t>(ram_[offset + i]) << (8U * i);
        high |= static_cast<std::uint64_t>(ram_[offset + 8 + i]) << (8U * i);
    }
    if (error) error->clear();
    return true;
}

bool Ps2EeMemoryMap::write128(std::uint32_t address, std::uint64_t low,
                              std::uint64_t high, std::string* error) {
    std::size_t offset = 0;
    if (!resolve_main_ram(address, 16, 16, offset, error)) return false;
    for (std::size_t i = 0; i < 8; ++i) {
        ram_[offset + i] = static_cast<std::uint8_t>((low >> (8U * i)) & 0xFFU);
        ram_[offset + 8 + i] = static_cast<std::uint8_t>((high >> (8U * i)) & 0xFFU);
    }
    if (error) error->clear();
    return true;
}

} // namespace retroshelf
