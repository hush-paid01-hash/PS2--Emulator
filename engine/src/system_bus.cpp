#include "retroshelf/system_bus.hpp"

#include <algorithm>

#include "retroshelf/ps2_ee_memory_map.hpp"

namespace retroshelf {

SystemBus::SystemBus(std::vector<std::uint8_t>& ee_ram) : ee_ram_(&ee_ram) {}
SystemBus::SystemBus(Ps2EeMemoryMap& ee_memory) : ee_memory_(&ee_memory) {}

bool SystemBus::valid_range(std::uint32_t address, std::size_t size) const {
    if (ee_memory_ != nullptr) {
        const std::uint64_t end = static_cast<std::uint64_t>(address) +
                                  static_cast<std::uint64_t>(size);
        return end <= (std::uint64_t{1} << 32U);
    }
    if (ee_ram_ == nullptr) return false;
    const auto start = static_cast<std::size_t>(address);
    return start <= ee_ram_->size() && size <= ee_ram_->size() - start;
}

bool SystemBus::read(std::uint32_t address, std::size_t size,
                     std::vector<std::uint8_t>& data, std::string* error) const {
    if (!valid_range(address, size)) {
        if (error != nullptr)
            *error = ee_memory_ != nullptr ? "read is outside EE address space"
                                           : "read is outside EE RAM";
        data.clear();
        return false;
    }

    if (ee_memory_ == nullptr) {
        const auto start = static_cast<std::size_t>(address);
        data.assign(ee_ram_->begin() + start, ee_ram_->begin() + start + size);
    } else {
        data.resize(size);
        std::size_t offset = 0;
        while (offset < size) {
            const std::uint32_t current = address + static_cast<std::uint32_t>(offset);
            const std::size_t remaining = size - offset;
            std::size_t width = 1;
            if ((current & 7U) == 0U && remaining >= 8U) width = 8;
            else if ((current & 3U) == 0U && remaining >= 4U) width = 4;
            else if ((current & 1U) == 0U && remaining >= 2U) width = 2;

            std::uint64_t value = 0;
            if (!ee_memory_->read(current, width, value, error)) {
                data.clear();
                return false;
            }
            for (std::size_t i = 0; i < width; ++i)
                data[offset + i] = static_cast<std::uint8_t>(value >> (i * 8U));
            offset += width;
        }
    }
    if (error != nullptr) error->clear();
    return true;
}

bool SystemBus::write(std::uint32_t address, const std::vector<std::uint8_t>& data,
                      std::string* error) {
    if (!valid_range(address, data.size())) {
        if (error != nullptr)
            *error = ee_memory_ != nullptr ? "write is outside EE address space"
                                           : "write is outside EE RAM";
        return false;
    }

    if (ee_memory_ == nullptr) {
        const auto start = static_cast<std::size_t>(address);
        std::copy(data.begin(), data.end(), ee_ram_->begin() + start);
    } else {
        std::size_t offset = 0;
        while (offset < data.size()) {
            const std::uint32_t current = address + static_cast<std::uint32_t>(offset);
            const std::size_t remaining = data.size() - offset;
            std::size_t width = 1;
            if ((current & 7U) == 0U && remaining >= 8U) width = 8;
            else if ((current & 3U) == 0U && remaining >= 4U) width = 4;
            else if ((current & 1U) == 0U && remaining >= 2U) width = 2;

            std::uint64_t value = 0;
            for (std::size_t i = 0; i < width; ++i)
                value |= static_cast<std::uint64_t>(data[offset + i]) << (i * 8U);
            if (!ee_memory_->write(current, width, value, error)) return false;
            offset += width;
        }
    }
    if (error != nullptr) error->clear();
    return true;
}

std::size_t SystemBus::ram_size() const {
    return ee_memory_ != nullptr ? ee_memory_->main_ram_size()
                                 : (ee_ram_ != nullptr ? ee_ram_->size() : 0U);
}

} // namespace retroshelf
