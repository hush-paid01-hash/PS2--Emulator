#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf {

// Bounded EE memory foundation with main RAM aliases, the BIOS ROM window,
// and the 16 KiB EE scratchpad. MMIO, TLB behavior, and hardware side effects
// remain outside this memory-only model.
class Ps2EeMemoryMap {
public:
    static constexpr std::size_t MainRamSize = 32U * 1024U * 1024U;
    static constexpr std::uint32_t MainRamBase = 0x00000000U;
    static constexpr std::uint32_t MainRamMirrorBase = 0x20000000U;
    static constexpr std::uint32_t Kseg0Base = 0x80000000U;
    static constexpr std::uint32_t Kseg1Base = 0xA0000000U;
    static constexpr std::uint32_t SegmentMask = 0x1FFFFFFFU;
    static constexpr std::uint32_t BiosRomBase = 0x1FC00000U;
    static constexpr std::size_t BiosRomWindowSize = 4U * 1024U * 1024U;
    static constexpr std::uint32_t ScratchpadBase = 0x70000000U;
    static constexpr std::size_t ScratchpadSize = 16U * 1024U;

    Ps2EeMemoryMap();
    explicit Ps2EeMemoryMap(std::size_t main_ram_size);

    std::size_t main_ram_size() const;
    std::vector<std::uint8_t>& main_ram();
    const std::vector<std::uint8_t>& main_ram() const;

    // Install a read-only BIOS image in the EE physical BIOS window. This only
    // maps bytes; it does not authenticate or execute firmware.
    bool load_bios_image(const std::vector<std::uint8_t>& image,
                         std::string* error = nullptr);
    std::size_t bios_rom_size() const;
    std::size_t scratchpad_size() const;

    // Resolve a virtual EE address to a main-RAM offset. False means the
    // address is not in one of the supported RAM aliases.
    bool resolve_main_ram(std::uint32_t address, std::size_t width,
                          std::size_t alignment, std::size_t& offset,
                          std::string* error = nullptr) const;

    bool read(std::uint32_t address, std::size_t width, std::uint64_t& value,
              std::string* error = nullptr) const;
    bool write(std::uint32_t address, std::size_t width, std::uint64_t value,
               std::string* error = nullptr);
    bool read128(std::uint32_t address, std::uint64_t& low, std::uint64_t& high,
                 std::string* error = nullptr) const;
    bool write128(std::uint32_t address, std::uint64_t low, std::uint64_t high,
                  std::string* error = nullptr);

private:
    bool resolve_bios_rom(std::uint32_t address, std::size_t width,
                          std::size_t& offset, std::string* error) const;
    bool resolve_scratchpad(std::uint32_t address, std::size_t width,
                            std::size_t& offset, std::string* error) const;

    std::vector<std::uint8_t> ram_;
    std::vector<std::uint8_t> bios_rom_;
    std::vector<std::uint8_t> scratchpad_;
};

} // namespace retroshelf
