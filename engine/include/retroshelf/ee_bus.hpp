#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace retroshelf::ee {

// First-stage RAM/bus model for isolated testing. This is not yet the complete
// PS2 physical memory map or device bus.
class EeBus {
public:
    static constexpr std::uint32_t RamBase = 0x00000000U;
    static constexpr std::size_t RamSize = 2U * 1024U * 1024U;

    EeBus();
    void reset();
    bool read8(std::uint32_t address, std::uint8_t& value) const;
    bool read32(std::uint32_t address, std::uint32_t& value) const;
    bool write8(std::uint32_t address, std::uint8_t value);
    bool write32(std::uint32_t address, std::uint32_t value);
    std::size_t ram_size() const;

private:
    std::vector<std::uint8_t> ram_;
};

} // namespace retroshelf::ee
