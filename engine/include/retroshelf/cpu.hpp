#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace retroshelf {

// Educational milestone only: this is not yet a complete PS2 Emotion Engine.
// The memory here is a small test RAM, not the PS2's real memory map.
class Cpu {
public:
    static constexpr std::size_t RegisterCount = 32;
    static constexpr std::size_t TestMemorySize = 64 * 1024;

    Cpu();
    void reset();
    bool step(std::uint32_t instruction, std::string* error = nullptr);

    std::uint64_t reg(std::size_t index) const;
    void set_reg(std::size_t index, std::uint64_t value);
    std::uint64_t pc() const;
    void set_pc(std::uint64_t value);

private:
    std::array<std::uint64_t, RegisterCount> gpr_{};
    std::array<std::uint8_t, TestMemorySize> memory_{};
    std::uint64_t pc_{0};
};

} // namespace retroshelf
