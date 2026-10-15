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
//
// COP0 foundation: only Status, Cause, EPC and BadVAddr are modeled, with MFC0, MTC0, ERET,
// SYSCALL and BREAK. Exceptions raised: Address Error (load/store), Syscall, Breakpoint and
// Reserved Instruction. Not modeled: branch delay slots (Cause.BD), TLB/refill vectors, ERL,
// Count/Compare, interrupts, Debug/performance registers, kernel/user privilege checks, and
// bus errors (out-of-range LW/SW fail with an error and no exception).
class Cpu {
public:
    enum Cop0Register : std::size_t { BadVAddr = 8, Status = 12, Cause = 13, Epc = 14 };
    enum ExceptionCode : std::uint32_t {
        AddressErrorLoad = 4, AddressErrorStore = 5, Syscall = 8, Breakpoint = 9,
        ReservedInstruction = 10
    };
    static constexpr std::uint32_t StatusExl = 1U << 1U;
    static constexpr std::uint32_t StatusBev = 1U << 22U;
    static constexpr std::uint32_t StatusWritableMask = 0x0040FF1FU; // IE, EXL, ERL-bit, KSU, IM, BEV
    static constexpr std::uint32_t CauseWritableMask = 0x00000300U;  // software IP0/IP1
    static constexpr std::uint32_t ResetStatus = StatusBev;           // simplification
    static constexpr std::uint64_t VectorBev = 0xFFFFFFFFBFC00380ULL;
    static constexpr std::uint64_t VectorNormal = 0xFFFFFFFF80000180ULL;

    static constexpr std::size_t RegisterCount = 32;

    explicit Cpu(SystemBus& bus);
    void reset();
    bool step(std::uint32_t instruction, std::string* error = nullptr);

    std::uint64_t reg(std::size_t index) const;
    void set_reg(std::size_t index, std::uint64_t value);
    std::uint64_t pc() const;
    void set_pc(std::uint64_t value);

    // Returns the 32-bit COP0 register value, or 0 for registers that are not modeled.
    std::uint32_t cop0(std::size_t index) const;
    bool cop0_supported(std::size_t index) const;
    std::uint32_t exception_code() const; // Cause.ExcCode

private:
    void raise(ExceptionCode code, std::uint64_t faulting_pc, std::uint32_t bad_vaddr = 0,
               bool set_bad_vaddr = false);
    std::array<std::uint64_t, RegisterCount> gpr_{};
    SystemBus& bus_;
    std::uint64_t pc_{0};
    std::uint32_t status_{ResetStatus};
    std::uint32_t cause_{0};
    std::uint32_t epc_{0};
    std::uint32_t bad_vaddr_{0};
};

} // namespace retroshelf
