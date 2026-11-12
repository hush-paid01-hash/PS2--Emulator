#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace retroshelf {

class Ps2EeMemoryMap;

class MmioDevice {
public:
    virtual ~MmioDevice() = default;
    virtual bool read(std::uint32_t offset, std::size_t size,
                      std::vector<std::uint8_t>& data,
                      std::string* error = nullptr) const = 0;
    virtual bool write(std::uint32_t offset, const std::vector<std::uint8_t>& data,
                       std::string* error = nullptr) = 0;
};

class SystemBus {
public:
    explicit SystemBus(std::vector<std::uint8_t>& ee_ram);
    explicit SystemBus(Ps2EeMemoryMap& ee_memory);

    bool map_mmio(std::uint32_t base, std::uint32_t size,
                  std::shared_ptr<MmioDevice> device, std::string* error = nullptr);
    bool read(std::uint32_t address, std::size_t size, std::vector<std::uint8_t>& data,
              std::string* error = nullptr) const;
    bool write(std::uint32_t address, const std::vector<std::uint8_t>& data,
               std::string* error = nullptr);
    std::size_t ram_size() const;

private:
    struct MmioMapping {
        std::uint32_t base;
        std::uint64_t end;
        std::shared_ptr<MmioDevice> device;
    };

    bool valid_range(std::uint32_t address, std::size_t size) const;
    const MmioMapping* find_mmio(std::uint32_t address, std::size_t size) const;
    bool valid_mmio_access(std::uint32_t address, std::size_t size,
                           std::string* error) const;

    std::vector<std::uint8_t>* ee_ram_ = nullptr;
    Ps2EeMemoryMap* ee_memory_ = nullptr;
    std::vector<MmioMapping> mmio_mappings_;
};

} // namespace retroshelf
