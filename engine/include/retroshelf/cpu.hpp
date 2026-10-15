#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "retroshelf/system_bus.hpp"

namespace retroshelf {

// Educational milestone only: this is not yet a complete PS2 Emotion Engine.
// LW/SW use the shared SystemBus; addresses are plain byte offsets into the bus's
// EE RAM (no KSEG/TLB translation, MMIO, or real PS2 memory map).
class Cpu {
public:
    static constexpr std::size_t RegisterCount = 32;

    explicit Cpu(SystemBus& bus);
    void reset();
    bool step(std::uint32_t instruction, std::string* error = nullptr);

    std::uint64_t reg(std::size_t index) const;
    void set_reg(std::size_t index, std::uint64_t value);
    std::uint64_t pc() const;
    void set_pc(std::uint64_t value);

private:
    std::array<std::uint64_t, RegisterCount> gpr_{};
    SystemBus& bus_;
    std::uint64_t pc_{0};
};

} // namespace retroshelf
