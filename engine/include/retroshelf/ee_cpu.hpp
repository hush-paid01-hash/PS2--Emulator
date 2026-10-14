#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace retroshelf::ee {

// Independent R5900/Emotion Engine core foundation.
// This is a deliberately incomplete instruction subset, not yet a full PS2 CPU.
struct Register128 {
    std::uint64_t lo{0};
    std::uint64_t hi{0};
};

class EeCpu {
public:
    static constexpr std::size_t GprCount = 32;

    EeCpu();
    void reset();

    // Executes one supplied instruction word at the current PC.
    // Returns false for unsupported instructions and records a diagnostic.
    bool step(std::uint32_t instruction, std::string* error = nullptr);

    Register128 gpr(std::size_t index) const;
    void set_gpr(std::size_t index, Register128 value);
    std::uint64_t pc() const;
    void set_pc(std::uint64_t value);
    bool branch_pending() const;
    std::uint64_t branch_target() const;

private:
    std::array<Register128, GprCount> gpr_{};
    std::uint64_t pc_{0};
    bool branch_pending_{false};
    std::uint64_t branch_target_{0};
};

} // namespace retroshelf::ee
