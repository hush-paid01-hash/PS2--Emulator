#include "retroshelf/ee_system_bus.hpp"

namespace retroshelf::ee {
EeSystemBus::EeSystemBus(EeBus& ram, EeHardware& hardware) : ram_(ram), hardware_(hardware) {}
bool EeSystemBus::read8(std::uint32_t address, std::uint8_t& value) const {
    std::uint32_t word = 0;
    const auto aligned = address & ~std::uint32_t{3};
    if (address >= EeHardware::MmioBase && address <= EeHardware::MmioBase + 0x14U) {
        if (!hardware_.read32(aligned, word)) return false;
        value = static_cast<std::uint8_t>((word >> ((address & 3U) * 8U)) & 0xFFU);
        return true;
    }
    return ram_.read8(address, value);
}
bool EeSystemBus::read32(std::uint32_t address, std::uint32_t& value) const {
    if (address >= EeHardware::MmioBase && address <= EeHardware::MmioBase + 0x14U)
        return hardware_.read32(address, value);
    return ram_.read32(address, value);
}
bool EeSystemBus::write8(std::uint32_t address, std::uint8_t value) {
    if (address >= EeHardware::MmioBase && address <= EeHardware::MmioBase + 0x14U) {
        std::uint32_t old = 0;
        const auto aligned = address & ~std::uint32_t{3};
        if (!hardware_.read32(aligned, old)) return false;
        const auto shift = (address & 3U) * 8U;
        const auto updated = (old & ~(0xFFU << shift)) | (static_cast<std::uint32_t>(value) << shift);
        return hardware_.write32(aligned, updated);
    }
    return ram_.write8(address, value);
}
bool EeSystemBus::write32(std::uint32_t address, std::uint32_t value) {
    if (address >= EeHardware::MmioBase && address <= EeHardware::MmioBase + 0x14U)
        return hardware_.write32(address, value);
    return ram_.write32(address, value);
}
} // namespace retroshelf::ee
