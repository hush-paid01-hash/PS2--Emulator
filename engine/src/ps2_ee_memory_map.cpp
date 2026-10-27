#include "retroshelf/ps2_ee_memory_map.hpp"

#include <utility>

namespace retroshelf {
namespace {
bool is_bios_window_address(std::uint32_t address) {
    const std::uint32_t physical = address >= Ps2EeMemoryMap::Kseg0Base && address < 0xC0000000U
        ? address & Ps2EeMemoryMap::SegmentMask : address;
    return physical >= Ps2EeMemoryMap::BiosRomBase &&
           physical < Ps2EeMemoryMap::BiosRomBase + Ps2EeMemoryMap::BiosRomWindowSize;
}
} // namespace

Ps2EeMemoryMap::Ps2EeMemoryMap() : ram_(MainRamSize, 0), scratchpad_(ScratchpadSize, 0) {}
Ps2EeMemoryMap::Ps2EeMemoryMap(std::size_t main_ram_size)
    : ram_(main_ram_size, 0), scratchpad_(ScratchpadSize, 0) {}
std::size_t Ps2EeMemoryMap::main_ram_size() const { return ram_.size(); }
std::vector<std::uint8_t>& Ps2EeMemoryMap::main_ram() { return ram_; }
const std::vector<std::uint8_t>& Ps2EeMemoryMap::main_ram() const { return ram_; }

bool Ps2EeMemoryMap::load_bios_image(const std::vector<std::uint8_t>& image,
                                     std::string* error) {
    if (image.empty()) {
        if (error) *error = "BIOS image is empty";
        return false;
    }
    if (image.size() > BiosRomWindowSize) {
        if (error) *error = "BIOS image exceeds the 4 MiB EE BIOS ROM window";
        return false;
    }
    bios_rom_ = image;
    if (error) error->clear();
    return true;
}

std::size_t Ps2EeMemoryMap::bios_rom_size() const { return bios_rom_.size(); }
std::size_t Ps2EeMemoryMap::scratchpad_size() const { return scratchpad_.size(); }

bool Ps2EeMemoryMap::resolve_scratchpad(std::uint32_t address, std::size_t width,
                                        std::size_t& offset, std::string* error) const {
    if (address < ScratchpadBase ||
        static_cast<std::uint64_t>(address) >=
            static_cast<std::uint64_t>(ScratchpadBase) + ScratchpadSize)
        return false;
    offset = static_cast<std::size_t>(address - ScratchpadBase);
    if (width > scratchpad_.size() || offset > scratchpad_.size() - width) {
        if (error) *error = "Memory access exceeds the EE scratchpad boundary";
        return false;
    }
    if (error) error->clear();
    return true;
}

bool Ps2EeMemoryMap::resolve_bios_rom(std::uint32_t address, std::size_t width,
                                      std::size_t& offset, std::string* error) const {
    std::uint32_t physical = address;
    if (address >= Kseg0Base && address < 0xC0000000U)
        physical = address & SegmentMask;
    if (physical < BiosRomBase || physical >= BiosRomBase + BiosRomWindowSize)
        return false;
    offset = static_cast<std::size_t>(physical - BiosRomBase);
    if (bios_rom_.empty()) {
        if (error) *error = "No BIOS image is mapped";
        return false;
    }
    if (width > bios_rom_.size() || offset > bios_rom_.size() - width) {
        if (error) *error = "BIOS read exceeds the loaded ROM image";
        return false;
    }
    if (error) error->clear();
    return true;
}

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

    // The accelerated uncached window starts at 0x30100000 and maps
    // physical RAM from offset 1 MiB through the end of the 32 MiB bank.
    // It is a distinct alias, not a modulo mirror of the low 1 MiB.
    if (physical >= MainRamAcceleratedBase &&
        physical < MainRamAcceleratedEnd) {
        offset = static_cast<std::size_t>(physical - 0x30000000U);
    } else {
        if (physical >= MainRamMirrorBase &&
            physical < MainRamMirrorBase + MainRamSize)
            physical -= MainRamMirrorBase;

        if (physical >= MainRamBase && physical < MainRamSize) {
            offset = static_cast<std::size_t>(physical);
        } else {
            return fail("Address is not in a supported EE main-RAM region");
        }
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
    if (width != 1 && width != 2 && width != 4 && width != 8) {
        if (error) *error = "Unsupported memory access width";
        return false;
    }
    if ((address & static_cast<std::uint32_t>(width - 1U)) != 0U) {
        if (error) *error = "Misaligned memory access";
        return false;
    }
    std::size_t offset = 0;
    if (address >= ScratchpadBase &&
        static_cast<std::uint64_t>(address) <
            static_cast<std::uint64_t>(ScratchpadBase) + ScratchpadSize) {
        if (!resolve_scratchpad(address, width, offset, error)) return false;
        value = 0;
        for (std::size_t i = 0; i < width; ++i)
            value |= static_cast<std::uint64_t>(scratchpad_[offset + i]) << (8U * i);
        if (error) error->clear();
        return true;
    }
    if (is_bios_window_address(address)) {
        if (!resolve_bios_rom(address, width, offset, error)) return false;
        value = 0;
        for (std::size_t i = 0; i < width; ++i)
            value |= static_cast<std::uint64_t>(bios_rom_[offset + i]) << (8U * i);
        if (error) error->clear();
        return true;
    }
    std::size_t ram_offset = 0;
    if (!resolve_main_ram(address, width, width, ram_offset, error)) return false;
    value = 0;
    for (std::size_t i = 0; i < width; ++i)
        value |= static_cast<std::uint64_t>(ram_[ram_offset + i]) << (8U * i);
    if (error) error->clear();
    return true;
}

bool Ps2EeMemoryMap::write(std::uint32_t address, std::size_t width,
                           std::uint64_t value, std::string* error) {
    if (width != 1 && width != 2 && width != 4 && width != 8) {
        if (error) *error = "Unsupported memory access width";
        return false;
    }
    if ((address & static_cast<std::uint32_t>(width - 1U)) != 0U) {
        if (error) *error = "Misaligned memory access";
        return false;
    }
    std::size_t scratchpad_offset = 0;
    if (address >= ScratchpadBase &&
        static_cast<std::uint64_t>(address) <
            static_cast<std::uint64_t>(ScratchpadBase) + ScratchpadSize) {
        if (!resolve_scratchpad(address, width, scratchpad_offset, error)) return false;
        for (std::size_t i = 0; i < width; ++i)
            scratchpad_[scratchpad_offset + i] =
                static_cast<std::uint8_t>((value >> (8U * i)) & 0xFFU);
        if (error) error->clear();
        return true;
    }
    std::size_t bios_offset = 0;
    if (resolve_bios_rom(address, width, bios_offset, nullptr)) {
        if (error) *error = "Writes to the EE BIOS ROM are not permitted";
        return false;
    }
    const auto canonical = [this](std::uint32_t value) {
        return value >= Kseg0Base && value < 0xC0000000U ? value & SegmentMask : value;
    };
    const std::uint32_t physical = canonical(address);
    if (physical >= BiosRomBase && physical < BiosRomBase + BiosRomWindowSize) {
        if (error) *error = bios_rom_.empty() ? "No BIOS image is mapped" : "BIOS write exceeds the loaded ROM image";
        return false;
    }
    std::size_t offset = 0;
    if (!resolve_main_ram(address, width, width, offset, error)) return false;
    for (std::size_t i = 0; i < width; ++i)
        ram_[offset + i] = static_cast<std::uint8_t>((value >> (8U * i)) & 0xFFU);
    if (error) error->clear();
    return true;
}

bool Ps2EeMemoryMap::read128(std::uint32_t address, std::uint64_t& low,
                             std::uint64_t& high, std::string* error) const {
    if ((address & 0xFU) != 0U) {
        if (error) *error = "Misaligned 128-bit memory access";
        return false;
    }
    std::size_t offset = 0;
    if (address >= ScratchpadBase &&
        static_cast<std::uint64_t>(address) <
            static_cast<std::uint64_t>(ScratchpadBase) + ScratchpadSize) {
        if (!resolve_scratchpad(address, 16, offset, error)) return false;
        low = 0; high = 0;
        for (std::size_t i = 0; i < 8; ++i) {
            low |= static_cast<std::uint64_t>(scratchpad_[offset + i]) << (8U * i);
            high |= static_cast<std::uint64_t>(scratchpad_[offset + 8 + i]) << (8U * i);
        }
        if (error) error->clear();
        return true;
    }
    if (is_bios_window_address(address)) {
        if (!resolve_bios_rom(address, 16, offset, error)) return false;
        low = 0;
        high = 0;
        for (std::size_t i = 0; i < 8; ++i) {
            low |= static_cast<std::uint64_t>(bios_rom_[offset + i]) << (8U * i);
            high |= static_cast<std::uint64_t>(bios_rom_[offset + 8 + i]) << (8U * i);
        }
        if (error) error->clear();
        return true;
    }
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
    if ((address & 0xFU) != 0U) {
        if (error) *error = "Misaligned 128-bit memory access";
        return false;
    }
    std::size_t scratchpad_offset = 0;
    if (address >= ScratchpadBase &&
        static_cast<std::uint64_t>(address) <
            static_cast<std::uint64_t>(ScratchpadBase) + ScratchpadSize) {
        if (!resolve_scratchpad(address, 16, scratchpad_offset, error)) return false;
        for (std::size_t i = 0; i < 8; ++i) {
            scratchpad_[scratchpad_offset + i] =
                static_cast<std::uint8_t>((low >> (8U * i)) & 0xFFU);
            scratchpad_[scratchpad_offset + 8 + i] =
                static_cast<std::uint8_t>((high >> (8U * i)) & 0xFFU);
        }
        if (error) error->clear();
        return true;
    }
    std::size_t bios_offset = 0;
    if (resolve_bios_rom(address, 16, bios_offset, nullptr)) {
        if (error) *error = "Writes to the EE BIOS ROM are not permitted";
        return false;
    }
    const std::uint32_t physical = address >= Kseg0Base && address < 0xC0000000U
        ? address & SegmentMask : address;
    if (physical >= BiosRomBase && physical < BiosRomBase + BiosRomWindowSize) {
        if (error) *error = bios_rom_.empty() ? "No BIOS image is mapped" : "BIOS write exceeds the loaded ROM image";
        return false;
    }
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
