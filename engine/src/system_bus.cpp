#include "retroshelf/system_bus.hpp"

#include <algorithm>

namespace retroshelf {

SystemBus::SystemBus(std::vector<std::uint8_t>& ee_ram) : ee_ram_(ee_ram) {}

bool SystemBus::valid_range(std::uint32_t address, std::size_t size) const {
    const auto start = static_cast<std::size_t>(address);
    return start <= ee_ram_.size() && size <= ee_ram_.size() - start;
}

bool SystemBus::read(std::uint32_t address, std::size_t size,
                     std::vector<std::uint8_t>& data, std::string* error) const {
    if (!valid_range(address, size)) {
        if (error != nullptr) {
            *error = "read is outside EE RAM";
        }
        data.clear();
        return false;
    }

    const auto start = static_cast<std::size_t>(address);
    data.assign(ee_ram_.begin() + start, ee_ram_.begin() + start + size);
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

bool SystemBus::write(std::uint32_t address, const std::vector<std::uint8_t>& data,
                      std::string* error) {
    if (!valid_range(address, data.size())) {
        if (error != nullptr) {
            *error = "write is outside EE RAM";
        }
        return false;
    }

    const auto start = static_cast<std::size_t>(address);
    std::copy(data.begin(), data.end(), ee_ram_.begin() + start);
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

std::size_t SystemBus::ram_size() const { return ee_ram_.size(); }

} // namespace retroshelf
