#include "retroshelf/cpu.hpp"
#include "retroshelf/ee_gif_dma.hpp"
#include "retroshelf/gs_packet_processor.hpp"
#include "retroshelf/system_bus.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr std::uint32_t Start = 1U << 8U;
constexpr std::uint32_t Chain = 1U << 2U;
constexpr std::uint32_t SwOp = (0x2BU << 26U) | (11U << 21U) | (12U << 16U);
constexpr std::uint32_t LwOp = (0x23U << 26U) | (11U << 21U) | (13U << 16U);

bool store_word(retroshelf::Cpu& cpu, std::uint32_t address, std::uint32_t value) {
    cpu.set_reg(11, address);
    cpu.set_reg(12, value);
    return cpu.step(SwOp);
}
} // namespace

int main() {
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << "\n"; ++failures; }
        else { std::cout << "PASS: " << label << "\n"; }
    };

    std::vector<std::uint8_t> ram(256, 0);
    retroshelf::SystemBus bus(ram);
    retroshelf::Cpu cpu(bus);

    // CNT tag at 0 (QWC=2, ID=1); GIF tag (NLOOP=1, EOP=1, PACKED, NREG=1, REGS=A+D) at 16,
    // one A+D qword at 32; END tag at 48. All words are written by CPU SW instructions.
    bool ok = store_word(cpu, 0, 2U | (1U << 28U));
    ok = ok && store_word(cpu, 16, 1U | (1U << 15U));
    ok = ok && store_word(cpu, 20, 1U << 28U);
    ok = ok && store_word(cpu, 24, 0xE);
    ok = ok && store_word(cpu, 32, 0xCAFEBABEU);
    ok = ok && store_word(cpu, 48, 7U << 28U);
    check(ok, "CPU writes DMA chain and GIF packet into shared EE RAM");

    retroshelf::GsPacketProcessor gs;
    std::vector<std::vector<std::uint8_t>> payloads;
    retroshelf::EeGifDma dma(bus, [&](const std::vector<std::uint8_t>& p) {
        payloads.push_back(p);
        gs.submit(p);
    });
    dma.registers.chcr = Start | Chain;
    dma.registers.tadr = 0;
    check(dma.start(), "GIF DMA starts on CPU-written chain");
    check(payloads.size() == 1 && payloads[0].size() == 32, "DMA delivered 2 qwords");
    check(payloads.size() == 1 && payloads[0].size() == 32 && payloads[0][0] == 1 &&
              payloads[0][16] == 0xBE && payloads[0][19] == 0xCA,
          "DMA payload matches CPU-written bytes");
    check(gs.finish_stream() && gs.error().empty(), "GS parser accepts packet");
    const auto packets = gs.take_packets();
    check(packets.size() == 1 && packets[0].tag_count == 1 && packets[0].bytes.size() == 32,
          "GS parser frames the CPU-written packet");

    // Writes through the bus are visible to CPU loads.
    check(bus.write(64, {0x78, 0x56, 0x34, 0x12}), "bus write succeeds");
    cpu.set_reg(11, 64);
    check(cpu.step(LwOp) && cpu.reg(13) == 0x12345678, "CPU LW observes bus write");

    // DMA consumed the same RAM the CPU used (no copy).
    check(ram[32] == 0xBE && ram[35] == 0xCA, "CPU writes landed in the backing EE RAM vector");

    std::string error;
    cpu.set_reg(11, ram.size());
    check(!cpu.step(LwOp, &error) && !error.empty(), "CPU access past EE RAM is rejected");
    return failures == 0 ? 0 : 1;
}
