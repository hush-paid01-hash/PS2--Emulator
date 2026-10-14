#include "retroshelf/ee_bus.hpp"

namespace retroshelf::ee {

EeBus::EeBus() : ram_(RamSize, 0) {}
void EeBus::reset() { for (auto& byte : ram_) byte = 0; }
std::size_t EeBus::ram_size() const { return ram_.size(); }

bool EeBus::read8(std::uint32_t address, std::uint8_t& value) const {
    if (address < RamBase || static_cast<std::uint64_t>(address - RamBase) >= ram_.size()) return false;
    value = ram_[address - RamBase];
    return true;
}

bool EeBus::read32(std::uint32_t address, std::uint32_t& value) const {
    if ((address & 3U) != 0U || static_cast<std::uint64_t>(address - RamBase) + 4U > ram_.size()) return false;
    const auto offset = static_cast<std::size_t>(address - RamBase);
    value = static_cast<std::uint32_t>(ram_[offset])
          | (static_cast<std::uint32_t>(ram_[offset + 1]) << 8U)
          | (static_cast<std::uint32_t>(ram_[offset + 2]) << 16U)
          | (static_cast<std::uint32_t>(ram_[offset + 3]) << 24U);
    return true;
}

bool EeBus::write8(std::uint32_t address, std::uint8_t value) {
    if (static_cast<std::uint64_t>(address - RamBase) >= ram_.size()) return false;
    ram_[address - RamBase] = value;
    return true;
}

bool EeBus::write32(std::uint32_t address, std::uint32_t value) {
    if ((address & 3U) != 0U || static_cast<std::uint64_t>(address - RamBase) + 4U > ram_.size()) return false;
    const auto offset = static_cast<std::size_t>(address - RamBase);
    ram_[offset] = static_cast<std::uint8_t>(value & 0xFFU);
    ram_[offset + 1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    ram_[offset + 2] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    ram_[offset + 3] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
    return true;
}

} // namespace retroshelf::ee
