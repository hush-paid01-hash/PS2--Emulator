#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace retroshelf {

// Educational first milestone only: this is not yet a complete PS2 Emotion Engine.
// It implements a small, testable subset of MIPS-style instructions.
class Cpu {
public:
    static constexpr std::size_t RegisterCount = 32;

    Cpu();
    void reset();
    bool step(std::uint32_t instruction, std::string* error = nullptr);

    std::uint64_t reg(std::size_t index) const;
    void set_reg(std::size_t index, std::uint64_t value);
    std::uint64_t pc() const;
    void set_pc(std::uint64_t value);

private:
    std::array<std::uint64_t, RegisterCount> gpr_{};
    std::uint64_t pc_{0};
};

} // namespace retroshelf
