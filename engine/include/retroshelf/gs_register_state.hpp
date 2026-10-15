#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace retroshelf {

// Small, deterministic backing store for the GS's 7-bit register address space.
// Register-specific side effects belong in the renderer/device layer.
class GsRegisterState {
public:
    static constexpr std::size_t RegisterCount = 128;

    GsRegisterState();

    // Returns false for addresses outside the GS register address space.
    bool write(std::uint8_t address, std::uint64_t value);
    bool read(std::uint8_t address, std::uint64_t& value) const;
    void reset();

    std::uint64_t write_count() const;

private:
    std::array<std::uint64_t, RegisterCount> registers_{};
    std::uint64_t write_count_{0};
};

} // namespace retroshelf
