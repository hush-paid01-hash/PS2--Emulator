#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>

namespace retroshelf::ee {

struct Register128 {
    std::uint64_t lo{0};
    std::uint64_t hi{0};
};

class EeCpu {
public:
    static constexpr std::size_t GprCount = 32;
    using ReadWord = std::function<bool(std::uint32_t, std::uint32_t&)>;
    using WriteWord = std::function<bool(std::uint32_t, std::uint32_t)>;

    EeCpu();
    void reset();
    bool step(std::uint32_t instruction, std::string* error = nullptr,
              const ReadWord& read_word = {}, const WriteWord& write_word = {});
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
