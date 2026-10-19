#include "retroshelf/cpu.hpp"
#include "retroshelf/ee_execution_runner.hpp"
#include "retroshelf/ee_gif_gs_runtime.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace {
void write_u64(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint64_t value) {
    for (std::size_t i = 0; i < 8; ++i)
        bytes[offset + i] = static_cast<std::uint8_t>(value >> (i * 8U));
}
std::uint32_t sw(unsigned rt, unsigned rs) {
    return (0x2BU << 26U) | (rs << 21U) | (rt << 16U);
}
}

int main() {
    retroshelf::Ps2EeMemoryMap memory(64U * 1024U);
    retroshelf::EeGifGsRuntime graphics(memory.main_ram(), 8, 8);
    assert(graphics.attach_memory_map(memory));

    // Five packed GIF register writes: PRIM, RGBAQ, then three vertices.
    const std::uint8_t addresses[] = {0x0, 0x1, 0x5, 0x5, 0x5};
    const std::uint64_t values[] = {
        0x3ULL, 0xFF0000FFULL,
        (static_cast<std::uint64_t>(1U << 4U) << 16U) | (1U << 4U),
        (static_cast<std::uint64_t>(1U << 4U) << 16U) | (6U << 4U),
        (static_cast<std::uint64_t>(6U << 4U) << 16U) | (1U << 4U)
    };
    for (std::size_t i = 0; i < 5; ++i) {
        const std::size_t offset = i * 32U;
        const std::uint64_t tag = 1ULL | (i == 4 ? (1ULL << 15U) : 0ULL) | (1ULL << 60U);
        write_u64(memory.main_ram(), offset, tag);
        write_u64(memory.main_ram(), offset + 8U, addresses[i]);
        write_u64(memory.main_ram(), offset + 16U, values[i]);
        write_u64(memory.main_ram(), offset + 24U, 0);
    }

    // A tiny EE program writes MADR, QWC and CHCR through the mapped MMIO
    // registers. The CHCR start write synchronously runs DMA and renders GS.
    constexpr std::uint32_t program = 0x80001000U;
    std::string error;
    assert(memory.write(program, 4, sw(9, 8), &error));
    assert(memory.write(program + 4U, 4, sw(11, 10), &error));
    assert(memory.write(program + 8U, 4, sw(13, 12), &error));

    retroshelf::Cpu cpu;
    cpu.set_pc(program);
    cpu.set_reg(8, 0x1000A010U); // GIF MADR
    cpu.set_reg(9, 0U);
    cpu.set_reg(10, 0x1000A020U); // GIF QWC
    cpu.set_reg(11, 10U);
    cpu.set_reg(12, 0x1000A000U); // GIF CHCR
    cpu.set_reg(13, 1U << 8U); // start, normal mode, EE RAM -> GIF

    retroshelf::EeExecutionRunner runner(cpu, memory);
    const auto result = runner.run(3);
    assert(result.instructions_executed == 3);
    assert(result.reason == retroshelf::EeExecutionRunner::StopReason::InstructionLimit);
    assert(graphics.dma().state() == retroshelf::EeGifDma::State::Completed);
    assert((graphics.dma().registers.chcr & (1U << 8U)) == 0U);
    retroshelf::GsFramebuffer::Rgba8 pixel{};
    assert(graphics.framebuffer().get_pixel(2, 2, pixel));
    assert(pixel.r == 255 && pixel.g == 0 && pixel.b == 0 && pixel.a == 255);

    // Reads through the CPU-visible MMIO map reflect completed hardware state.
    std::uint64_t chcr = 0;
    assert(memory.read(0x1000A000U, 4, chcr, &error));
    assert((chcr & (1U << 8U)) == 0U);
    return 0;
}
