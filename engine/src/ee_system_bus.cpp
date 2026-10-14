#include "retroshelf/ee_system_bus.hpp"

#include <algorithm>
#include <sstream>

namespace retroshelf::ee {
namespace {
std::uint32_t bios_physical_address(std::uint32_t address) {
    // EE KSEG1 BIOS reset-vector alias: 0xBFC00000 -> 0x1FC00000.
    if (address >= 0xBFC00000U && address < 0xC0000000U)
        return address - 0xA0000000U;
    return address;
}
std::uint32_t ram_physical_address(std::uint32_t address) {
    // EE cached/uncached direct-mapped RAM aliases (KSEG0/KSEG1).
    if (address >= 0x80000000U && address < 0xC0000000U)
        return address & 0x1FFFFFFFU;
    return address;
}
bool in_bios_window(std::uint32_t address) {
    const auto physical = bios_physical_address(address);
    return physical >= EeSystemBus::BiosBase &&
           static_cast<std::uint64_t>(physical) <
               static_cast<std::uint64_t>(EeSystemBus::BiosBase) + EeSystemBus::BiosSize;
}
}

EeSystemBus::EeSystemBus(EeBus& ram, EeHardware& hardware) : ram_(ram), hardware_(hardware) {}

bool EeSystemBus::load_bios(const std::vector<std::uint8_t>& image, std::string* error) {
    if (image.empty() || image.size() > BiosSize) {
        if (error) *error = "BIOS image must contain between 1 byte and 4 MiB";
        return false;
    }
    // Replace the image only after validation, so a rejected file preserves the prior BIOS.
    bios_ = image;
    if (error) error->clear();
    return true;
}

void EeSystemBus::clear_bios() { bios_.clear(); }
bool EeSystemBus::bios_loaded() const { return !bios_.empty(); }

bool EeSystemBus::read8(std::uint32_t address, std::uint8_t& value) const {
    if (in_bios_window(address)) {
        if (bios_.empty()) return false;
        const auto offset = static_cast<std::size_t>(bios_physical_address(address) - BiosBase);
        if (offset >= bios_.size()) return false;
        value = bios_[offset];
        return true;
    }
    std::uint32_t word = 0;
    const auto aligned = address & ~std::uint32_t{3};
    if (address >= EeHardware::MmioBase && address <= EeHardware::MmioBase + 0x14U) {
        if (!hardware_.read32(aligned, word)) return false;
        value = static_cast<std::uint8_t>((word >> ((address & 3U) * 8U)) & 0xFFU);
        return true;
    }
    return ram_.read8(ram_physical_address(address), value);
}

bool EeSystemBus::read32(std::uint32_t address, std::uint32_t& value) const {
    if ((address & 3U) != 0U) return false;
    if (in_bios_window(address)) {
        if (bios_.empty()) return false;
        const auto offset = static_cast<std::size_t>(bios_physical_address(address) - BiosBase);
        if (offset + 4U > bios_.size()) return false;
        value = static_cast<std::uint32_t>(bios_[offset]) |
                (static_cast<std::uint32_t>(bios_[offset + 1]) << 8U) |
                (static_cast<std::uint32_t>(bios_[offset + 2]) << 16U) |
                (static_cast<std::uint32_t>(bios_[offset + 3]) << 24U);
        return true;
    }
    if (address >= EeHardware::MmioBase && address <= EeHardware::MmioBase + 0x14U)
        return hardware_.read32(address, value);
    return ram_.read32(ram_physical_address(address), value);
}

bool EeSystemBus::write8(std::uint32_t address, std::uint8_t value) {
    if (in_bios_window(address)) return false; // ROM is never writable by the emulated CPU.
    if (address >= EeHardware::MmioBase && address <= EeHardware::MmioBase + 0x14U) {
        std::uint32_t old = 0;
        const auto aligned = address & ~std::uint32_t{3};
        if (!hardware_.read32(aligned, old)) return false;
        const auto shift = (address & 3U) * 8U;
        const auto updated = (old & ~(0xFFU << shift)) | (static_cast<std::uint32_t>(value) << shift);
        return hardware_.write32(aligned, updated);
    }
    return ram_.write8(ram_physical_address(address), value);
}

bool EeSystemBus::write32(std::uint32_t address, std::uint32_t value) {
    if (in_bios_window(address)) return false;
    if (address >= EeHardware::MmioBase && address <= EeHardware::MmioBase + 0x14U)
        return hardware_.write32(address, value);
    return ram_.write32(ram_physical_address(address), value);
}
} // namespace retroshelf::ee
