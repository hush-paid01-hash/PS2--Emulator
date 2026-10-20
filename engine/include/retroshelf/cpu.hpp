#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf { class Ps2EeMemoryMap; }

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
    bool step_from_ram(std::vector<std::uint8_t>& ram,
                       std::uint32_t ram_base,
                       std::string* error = nullptr);
    bool step_from_ee_memory(Ps2EeMemoryMap& memory,
                             std::string* error = nullptr);

    std::uint64_t reg(std::size_t index) const;
    void set_reg(std::size_t index, std::uint64_t value);
    std::uint64_t reg_high(std::size_t index) const;
    void set_reg_high(std::size_t index, std::uint64_t value);
    std::uint64_t pc() const;
    void set_pc(std::uint64_t value);

    // Minimal COP0 subset: Status, Cause, EPC and BadVAddr are modelled.
    static constexpr std::uint32_t CauseBd = 1U << 31U;
    static constexpr std::uint32_t StatusExl = 1U << 1U;
    static constexpr std::uint32_t ExcSyscall = 8;
    static constexpr std::uint32_t ExcBreakpoint = 9;
    static constexpr std::uint32_t ExcAddressErrorLoad = 4;
    static constexpr std::uint32_t ExcAddressErrorStore = 5;
    static constexpr std::uint32_t ExcReservedInstruction = 10;
    static constexpr std::uint32_t ExcOverflow = 12;
    // Simplified: always 0x80000180 (no BEV/TLB vector selection).
    static constexpr std::uint64_t GeneralExceptionVector = 0x80000180ULL;

    std::uint32_t cop0_status() const;
    std::uint32_t cop0_cause() const;
    std::uint64_t cop0_epc() const;
    std::uint64_t cop0_bad_vaddr() const;
    bool in_delay_slot() const; // true if the next step() runs a branch delay slot
    std::uint64_t next_pc() const;
    std::uint64_t hi() const;
    std::uint64_t lo() const;

private:
    std::array<std::uint64_t, RegisterCount> gpr_{};
    std::array<std::uint64_t, RegisterCount> gpr_high_{};
    std::array<std::uint8_t, TestMemorySize> memory_{};
    void raise_exception(std::uint32_t code, std::uint64_t fault_pc, bool in_delay_slot);
    std::vector<std::uint8_t>* active_ram_{nullptr};
    std::uint32_t active_ram_base_{0};
    Ps2EeMemoryMap* active_ee_memory_{nullptr};

    std::uint64_t pc_{0};
    std::uint64_t next_pc_{4};
    bool delay_slot_pending_{false};
    std::uint64_t delay_branch_pc_{0};
    std::array<std::uint32_t, RegisterCount> cop0_regs_{};
    std::uint32_t status_{0};
    std::uint32_t cause_{0};
    std::uint64_t epc_{0};
    std::uint64_t bad_vaddr_{0};
    std::uint64_t hi_{0};
    std::uint64_t lo_{0};
};

} // namespace retroshelf
