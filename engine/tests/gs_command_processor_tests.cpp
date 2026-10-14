#include "retroshelf/gs_command_processor.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    using retroshelf::gs::GsCommandProcessor;
    GsCommandProcessor gs(64, 64);
    // GIFtag: EOP, PACKED, NREG=1, descriptor=A+D.
    std::vector<std::uint64_t> stream;
    const auto tag = [](std::uint16_t nloop, bool eop) {
        return static_cast<std::uint64_t>(nloop) |
               (static_cast<std::uint64_t>(eop) << 15U) |
               (static_cast<std::uint64_t>(0U) << 58U) |
               (static_cast<std::uint64_t>(1U) << 60U);
    };
    const auto ad = [&stream](std::uint64_t value, std::uint8_t address) {
        stream.push_back(value);
        stream.push_back(address);
    };
    stream.push_back(tag(5, true)); stream.push_back(0xE);
    ad(3U, 0x00); // PRIM=triangle list
    ad(0xFF000000U, 0x01); // RGBAQ packed color in upper word for this prototype
    ad(16U | (16U << 16U), 0x05);
    ad(48U | (16U << 16U), 0x05);
    ad(16U | (48U << 16U), 0x05);
    std::string error;
    if (!gs.execute_gif(stream.data(), stream.size() / 2U, &error)) {
        std::cerr << error << '\n'; return 1;
    }
    if (gs.triangles_drawn() != 1) return 2;
    if (gs.register_value(0x00) != 3U) return 3;
    // Truncated stream must fail rather than silently accept a partial command.
    if (gs.execute_gif(stream.data(), 2, &error)) return 4;
    std::cout << "GS command processor tests passed\n";
    return 0;
}
