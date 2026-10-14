#pragma once
#include "retroshelf/ee_bus.hpp"
#include "retroshelf/ee_hardware.hpp"
#include <cstdint>

namespace retroshelf::ee {

// Prototype system bus dispatcher. RAM and project-local device registers are
// routed separately; this is not yet the PS2's complete physical address map.
class EeSystemBus {
public:
    EeSystemBus(EeBus& ram, EeHardware& hardware);
    bool read8(std::uint32_t address, std::uint8_t& value) const;
    bool read32(std::uint32_t address, std::uint32_t& value) const;
    bool write8(std::uint32_t address, std::uint8_t value);
    bool write32(std::uint32_t address, std::uint32_t value);
private:
    EeBus& ram_;
    EeHardware& hardware_;
};
} // namespace retroshelf::ee
