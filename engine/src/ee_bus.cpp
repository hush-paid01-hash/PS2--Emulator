#include "retroshelf/ee_bus.hpp"
namespace retroshelf::ee {
namespace {
bool ram_offset(std::uint32_t address, std::size_t size, std::size_t& offset) {
    std::uint32_t physical = address;
    if ((address >= 0x80000000U && address <= 0x9FFFFFFFU) ||
        (address >= 0xA0000000U && address <= 0xBFFFFFFFU)) physical = address & 0x1FFFFFFFU;
    if (physical >= size) return false;
    offset = static_cast<std::size_t>(physical);
    return true;
}
}
EeBus::EeBus() : ram_(RamSize, 0) {}
void EeBus::reset() { for (auto& byte : ram_) byte = 0; }
std::size_t EeBus::ram_size() const { return ram_.size(); }
bool EeBus::read8(std::uint32_t address, std::uint8_t& value) const {
    std::size_t offset = 0;
    if (!ram_offset(address, ram_.size(), offset)) return false;
    value = ram_[offset]; return true;
}
bool EeBus::read32(std::uint32_t address, std::uint32_t& value) const {
    std::size_t offset = 0;
    if ((address & 3U) != 0U || !ram_offset(address, ram_.size(), offset) || offset + 4U > ram_.size()) return false;
    value = static_cast<std::uint32_t>(ram_[offset]) | (static_cast<std::uint32_t>(ram_[offset+1]) << 8U)
          | (static_cast<std::uint32_t>(ram_[offset+2]) << 16U) | (static_cast<std::uint32_t>(ram_[offset+3]) << 24U);
    return true;
}
bool EeBus::write8(std::uint32_t address, std::uint8_t value) {
    std::size_t offset = 0;
    if (!ram_offset(address, ram_.size(), offset)) return false;
    ram_[offset] = value; return true;
}
bool EeBus::write32(std::uint32_t address, std::uint32_t value) {
    std::size_t offset = 0;
    if ((address & 3U) != 0U || !ram_offset(address, ram_.size(), offset) || offset + 4U > ram_.size()) return false;
    ram_[offset] = static_cast<std::uint8_t>(value);
    ram_[offset+1] = static_cast<std::uint8_t>(value >> 8U);
    ram_[offset+2] = static_cast<std::uint8_t>(value >> 16U);
    ram_[offset+3] = static_cast<std::uint8_t>(value >> 24U);
    return true;
}
} // namespace retroshelf::ee
