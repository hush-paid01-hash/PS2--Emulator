#import "RetroShelfBridge.h"
#include "retroshelf/ee_machine.hpp"
#include "retroshelf/gs_gif_processor.hpp"
#include <string>
#include <fstream>
#include <iterator>
#include <vector>

namespace { retroshelf::ee::EeMachine diagnostic_machine; bool diagnostic_loaded = false; }

extern "C" const char *rs_inspect_ps2_disc(const char *iso_path) {
    static std::string message;
    if (iso_path == nullptr || *iso_path == 0) {
        message = "No disc path was provided.";
        return message.c_str();
    }
    retroshelf::media::Ps2BootInfo boot;
    std::string error;
    if (!diagnostic_machine.load_ps2_disc(iso_path, &boot, &error)) {
        diagnostic_loaded = false;
        message = "Disc/ELF load failed: " + error;
        return message.c_str();
    }
    diagnostic_loaded = true;
    message = "ELF segments loaded into prototype EE RAM. Entry point: 0x";
    constexpr char hex[] = "0123456789ABCDEF";
    const auto entry = static_cast<std::uint32_t>(diagnostic_machine.cpu().pc());
    for (int shift = 28; shift >= 0; shift -= 4)
        message.push_back(hex[(entry >> shift) & 0xFU]);
    message += ". Full PS2 boot, BIOS execution, accurate hardware, and graphics are not implemented.";
    return message.c_str();
}

extern "C" const char *rs_run_ee_diagnostic(unsigned long long instruction_budget) {
    static std::string message;
    if (!diagnostic_loaded) {
        message = "Choose a valid PS2 ISO first; no executable is loaded.";
        return message.c_str();
    }
    std::string error;
    const auto executed = diagnostic_machine.run(static_cast<std::size_t>(instruction_budget), &error);
    message = "EE diagnostic executed " + std::to_string(executed) + " instruction(s); PC=0x";
    constexpr char hex[] = "0123456789ABCDEF";
    const auto pc = static_cast<std::uint32_t>(diagnostic_machine.cpu().pc());
    for (int shift = 28; shift >= 0; shift -= 4)
        message.push_back(hex[(pc >> shift) & 0xFU]);
    if (!error.empty()) message += ". Stopped: " + error;
    else message += ". Instruction budget reached. This is a CPU diagnostic, not gameplay.";
    return message.c_str();
}

extern "C" const char *rs_load_ps2_bios(const char *bios_path) {
    static std::string message;
    if (bios_path == nullptr || *bios_path == 0) {
        message = "No BIOS path was provided.";
        return message.c_str();
    }
    std::ifstream input(bios_path, std::ios::binary);
    if (!input) {
        message = "Could not open the selected BIOS file.";
        return message.c_str();
    }
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                                    std::istreambuf_iterator<char>());
    std::string error;
    if (!diagnostic_machine.load_bios(std::move(bytes), &error)) {
        message = "BIOS rejected: " + error;
        return message.c_str();
    }
    diagnostic_loaded = false;
    message = "BIOS image loaded into the prototype EE system bus and reset vector. Full boot and game execution are still under development.";
    return message.c_str();
}


extern "C" int rs_render_gif_pipeline_preview(unsigned char *rgba_bytes,
                                               unsigned long long capacity,
                                               unsigned int width,
                                               unsigned int height) {
    const auto pixel_count = static_cast<unsigned long long>(width) * height;
    if (!rgba_bytes || width == 0 || height == 0 ||
        pixel_count > capacity / 4ULL || pixel_count > 1024ULL * 1024ULL) return 0;
    retroshelf::gs::SoftwareRenderer renderer(width, height);
    renderer.clear(0x101827FFU);
    // Synthetic test stream: A+D writes for PRIM, RGBAQ and three XYZ2 vertices.
    // This exercises the parser/register/rasterizer path, not a game or PS2 boot.
    const std::uint64_t stream[] = {
        1ULL | (1ULL << 60U), 0xEULL, 3ULL, 0ULL,
        1ULL | (1ULL << 60U), 0xEULL, 0xFFFFFFFFULL, 0x01ULL,
        3ULL | (1ULL << 15U) | (1ULL << 60U), 0xEULL,
        (static_cast<std::uint64_t>(width / 2U) * 16ULL) |
            (static_cast<std::uint64_t>(height / 8U) * 16ULL << 16U), 0x05ULL,
        (static_cast<std::uint64_t>(width - width / 8U) * 16ULL) |
            (static_cast<std::uint64_t>(height - height / 8U) * 16ULL << 16U), 0x05ULL,
        (static_cast<std::uint64_t>(width / 8U) * 16ULL) |
            (static_cast<std::uint64_t>(height - height / 8U) * 16ULL << 16U), 0x05ULL
    };
    std::string error;
    if (!retroshelf::gs::process_gif_stream(stream, 8, renderer, nullptr, &error)) return 0;
    const auto &pixels = renderer.pixels();
    for (std::size_t i = 0; i < pixels.size(); ++i) {
        const auto p = pixels[i];
        rgba_bytes[i * 4U] = static_cast<unsigned char>((p >> 24U) & 0xFFU);
        rgba_bytes[i * 4U + 1U] = static_cast<unsigned char>((p >> 16U) & 0xFFU);
        rgba_bytes[i * 4U + 2U] = static_cast<unsigned char>((p >> 8U) & 0xFFU);
        rgba_bytes[i * 4U + 3U] = static_cast<unsigned char>(p & 0xFFU);
    }
    return 1;
}
