#include "retroshelf/system_bus.hpp"

#include <algorithm>
#include <utility>

#include "retroshelf/ps2_ee_memory_map.hpp"

namespace retroshelf {

namespace {
std::uint32_t mmio_physical_address(std::uint32_t address) {
    // KSEG0/KSEG1 are direct-mapped aliases of the same physical EE bus.
    if (address >= 0x80000000U && address < 0xC0000000U)
        return address & 0x1FFFFFFFU;
    return address;
}
} // namespace

SystemBus::SystemBus(std::vector<std::uint8_t>& ee_ram) : ee_ram_(&ee_ram) {}
SystemBus::SystemBus(Ps2EeMemoryMap& ee_memory) : ee_memory_(&ee_memory) {}

bool SystemBus::map_mmio(std::uint32_t base, std::uint32_t size,
                         std::shared_ptr<MmioDevice> device, std::string* error) {
    if (size == 0 || device == nullptr) {
        if (error != nullptr) *error = "MMIO mapping requires a non-zero size and a device";
        return false;
    }

    const std::uint64_t end = static_cast<std::uint64_t>(base) + size;
    if (end > (std::uint64_t{1} << 32U)) {
        if (error != nullptr) *error = "MMIO mapping wraps the 32-bit address space";
        return false;
    }

    for (const auto& mapping : mmio_mappings_) {
        if (static_cast<std::uint64_t>(base) < mapping.end &&
            static_cast<std::uint64_t>(mapping.base) < end) {
            if (error != nullptr) *error = "MMIO mapping overlaps an existing device";
            return false;
        }
    }

    mmio_mappings_.push_back({base, end, std::move(device)});
    std::sort(mmio_mappings_.begin(), mmio_mappings_.end(),
              [](const MmioMapping& left, const MmioMapping& right) {
                  return left.base < right.base;
              });
    if (error != nullptr) error->clear();
    return true;
}

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

const SystemBus::MmioMapping* SystemBus::find_mmio(std::uint32_t address,
                                                   std::size_t size) const {
    const std::uint64_t start = address;
    const std::uint64_t end = start + size;
    for (const auto& mapping : mmio_mappings_) {
        if (start >= mapping.base && start < mapping.end)
            return end <= mapping.end ? &mapping : nullptr;
    }
    return nullptr;
}

bool SystemBus::valid_mmio_access(std::uint32_t address, std::size_t size,
                                  std::string* error) const {
    if (size != 1 && size != 2 && size != 4 && size != 8) {
        if (error != nullptr) *error = "MMIO access width must be 1, 2, 4, or 8 bytes";
        return false;
    }
    if (address % size != 0) {
        if (error != nullptr) *error = "MMIO access is not naturally aligned";
        return false;
    }
    return true;
}

bool SystemBus::read(std::uint32_t address, std::size_t size,
                     std::vector<std::uint8_t>& data, std::string* error) const {
    const std::uint32_t device_address = mmio_physical_address(address);
    if (const auto* mapping = find_mmio(device_address, size)) {
        if (!valid_mmio_access(address, size, error)) {
            data.clear();
            return false;
        }
        if (!mapping->device->read(device_address - mapping->base, size, data, error) ||
            data.size() != size) {
            if (error != nullptr && error->empty())
                *error = "MMIO device returned an invalid read width";
            data.clear();
            return false;
        }
        if (error != nullptr) error->clear();
        return true;
    }

    for (const auto& mapping : mmio_mappings_) {
        if (device_address >= mapping.base && static_cast<std::uint64_t>(device_address) < mapping.end) {
            data.clear();
            if (error != nullptr) *error = "read crosses the end of an MMIO mapping";
            return false;
        }
    }

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
    const std::uint32_t device_address = mmio_physical_address(address);
    if (const auto* mapping = find_mmio(device_address, data.size())) {
        if (!valid_mmio_access(address, data.size(), error)) return false;
        if (!mapping->device->write(device_address - mapping->base, data, error)) return false;
        if (error != nullptr) error->clear();
        return true;
    }

    for (const auto& mapping : mmio_mappings_) {
        if (device_address >= mapping.base && static_cast<std::uint64_t>(device_address) < mapping.end) {
            if (error != nullptr) *error = "write crosses the end of an MMIO mapping";
            return false;
        }
    }

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
