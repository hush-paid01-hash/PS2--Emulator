#include "retroshelf/cpu.hpp"
#include "retroshelf/ee_execution_runner.hpp"
#include "retroshelf/ee_interrupt_controller.hpp"
#include "retroshelf/ps2_ee_memory_map.hpp"
#include "retroshelf/system_bus.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>

int main() {
    retroshelf::Ps2EeMemoryMap memory(64U * 1024U);
    retroshelf::SystemBus bus(memory);
    auto intc = std::make_shared<retroshelf::EeInterruptController>();
    std::string error;
    assert(bus.map_mmio(retroshelf::EeInterruptController::RegisterBase,
                        retroshelf::EeInterruptController::RegisterSize,
                        intc, &error));

    constexpr std::uint32_t program = 0x80001000U;
    // LUI t0, 0x1001; ADDIU t1, zero, 4;
    // SW t1, -4080(t0); LW t2, -4080(t0).
    constexpr std::uint32_t instructions[] = {
        0x3C081001U,
        0x24090004U,
        0xAD09F010U,
        0x8D0AF010U
    };
    for (std::size_t i = 0; i < 4; ++i)
        assert(memory.write(program + static_cast<std::uint32_t>(i * 4U),
                            4, instructions[i], &error));

    retroshelf::Cpu cpu;
    cpu.set_pc(program);
    retroshelf::EeExecutionRunner runner(cpu, memory, bus);
    const auto result = runner.run(4);
    assert(result.instructions_executed == 4U);
    assert(result.reason == retroshelf::EeExecutionRunner::StopReason::InstructionLimit);
    assert(intc->mask() == 4U);
    assert(cpu.reg(10) == 4U);
    assert(cpu.pc() == program + 16U);

    // The same mapped device must be reached by a load after its write.
    assert(intc->raise_interrupt(2));
    cpu.reset();
    cpu.set_pc(program);
    assert(runner.run(1).instructions_executed == 1U); // restore MMIO base register
    cpu.set_pc(program + 12U);
    assert(runner.run(1).instructions_executed == 1U);
    assert(cpu.reg(10) == 4U);

    // A sign-extended KSEG1 address must resolve to the same physical INTC.
    constexpr std::uint32_t kernel_program = program + 0x20U;
    constexpr std::uint32_t kernel_instructions[] = {
        0x3C08B001U, // LUI t0, 0xB001 -> sign-extended KSEG1 address
        0x24090008U, // ADDIU t1, zero, 8
        0xAD09F010U, // SW t1, -4080(t0) -> 0xB000F010
        0x8D0AF010U  // LW t2, -4080(t0)
    };
    for (std::size_t i = 0; i < 4; ++i)
        assert(memory.write(kernel_program + static_cast<std::uint32_t>(i * 4U),
                            4, kernel_instructions[i], &error));
    cpu.reset();
    cpu.set_pc(kernel_program);
    const auto kernel_result = runner.run(4);
    assert(kernel_result.instructions_executed == 4U);
    assert(intc->mask() == 12U);
    assert(cpu.reg(10) == 12U);

    // Direct KSEG0 and KSEG1 bus accesses share the same mapped device.
    std::vector<std::uint8_t> mask_bytes;
    assert(bus.read(0x9000F010U, 4, mask_bytes, &error));
    assert(mask_bytes.size() == 4U);
    assert(bus.read(0xB000F010U, 4, mask_bytes, &error));
    assert(mask_bytes.size() == 4U);

    return 0;
}
