#pragma once
#include "retroshelf/ee_bus.hpp"
#include "retroshelf/ee_hardware.hpp"
#include "retroshelf/ee_gif_dma.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace retroshelf::ee {
class EeSystemBus {
public:
    static constexpr std::uint32_t BiosBase = 0x1FC00000U;
    static constexpr std::size_t BiosSize = 4U * 1024U * 1024U;
    EeSystemBus(EeBus& ram, EeHardware& hardware);
    void attach_gif_dma(EeGifDma* dma);
    bool load_bios(const std::vector<std::uint8_t>& image, std::string* error = nullptr);
    void clear_bios();
    bool bios_loaded() const;
    bool read8(std::uint32_t address, std::uint8_t& value) const;
    bool read32(std::uint32_t address, std::uint32_t& value) const;
    bool write8(std::uint32_t address, std::uint8_t value);
    bool write32(std::uint32_t address, std::uint32_t value);
private:
    EeBus& ram_;
    EeHardware& hardware_;
    EeGifDma* gif_dma_{nullptr};
    std::vector<std::uint8_t> bios_;
};
} // namespace retroshelf::ee
