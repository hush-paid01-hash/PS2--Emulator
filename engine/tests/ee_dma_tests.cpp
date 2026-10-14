#include "retroshelf/ee_dma.hpp"
#include <cstdint>
#include <iostream>

int main() {
    retroshelf::ee::EeBus bus;
    retroshelf::ee::EeDma dma;
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << '\n'; ++failures; }
        else { std::cout << "PASS: " << label << '\n'; }
    };
    check(bus.write32(0x100, 0x44332211U), "prepare source word");
    check(dma.transfer(bus, 0x100, 0x200, 4), "copy memory through DMA helper");
    std::uint32_t value = 0;
    check(bus.read32(0x200, value) && value == 0x44332211U, "destination receives copied word");
    check(dma.transfer(bus, 0x200, 0x201, 4), "overlapping copy succeeds safely");
    std::uint8_t byte = 0;
    check(bus.read8(0x201, byte) && byte == 0x11, "overlapping copy uses original source bytes");
    check(!dma.transfer(bus, static_cast<std::uint32_t>(bus.ram_size() - 1), 0x300, 4),
          "out-of-range source is rejected");
    check(!dma.transfer(bus, 0x100, static_cast<std::uint32_t>(bus.ram_size() - 1), 4),
          "out-of-range destination is rejected");
    check(dma.transfer(bus, 0x100, 0x300, 0), "zero-byte transfer is valid");
    return failures == 0 ? 0 : 1;
}
