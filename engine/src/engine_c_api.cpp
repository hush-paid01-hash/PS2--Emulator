#include "retroshelf/engine_c_api.h"

#include "retroshelf/cpu.hpp"
#include "retroshelf/gs_framebuffer.hpp"
#include "retroshelf/gs_gif_pipeline.hpp"
#include "retroshelf/gs_packet_processor.hpp"
#include "retroshelf/iso9660_disc.hpp"
#include "retroshelf/ps2_disc_execution.hpp"
#include "retroshelf/ps2_ee_memory_map.hpp"
#include "retroshelf/ps2_boot_image.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

namespace {
retroshelf::GsFramebuffer g_preview_framebuffer;
bool g_preview_ready = false;

void write_u64_le(std::vector<std::uint8_t>& bytes, std::size_t offset,
                  std::uint64_t value) {
    for (std::size_t i = 0; i < 8; ++i) {
        bytes[offset + i] = static_cast<std::uint8_t>(value >> (i * 8U));
    }
}

// Build a small, synthetic PACKED GIF stream so the native preview exercises
// the same GIF decoder -> GS register state -> primitive renderer path as the
// current pipeline tests. It is still a test pattern, not a PS2 game's output.
std::vector<std::uint8_t> make_preview_gif_stream() {
    constexpr std::size_t tag_bytes = 32;
    constexpr std::size_t tag_count = 5;
    std::vector<std::uint8_t> bytes(tag_bytes * tag_count, 0);
    const std::uint8_t addresses[tag_count] = {0x0, 0x1, 0x5, 0x5, 0x5};
    const auto fixed = [](std::uint64_t pixels) {
        return pixels << 4U; // GS XYZ coordinates are 12.4 fixed-point here.
    };
    const std::uint64_t values[tag_count] = {
        0x3ULL,                    // PRIM: triangle list
        0xFF00DC00ULL,             // RGBAQ: opaque cyan in this register model
        (fixed(28) << 16U) | fixed(160),
        (fixed(202) << 16U) | fixed(38),
        (fixed(202) << 16U) | fixed(282)
    };

    for (std::size_t i = 0; i < tag_count; ++i) {
        const bool last = i + 1U == tag_count;
        // NLOOP=1, EOP on the final tag, FLG=PACKED, NREG=1.
        const std::uint64_t tag = 1ULL | (last ? (1ULL << 15U) : 0ULL) |
                                  (1ULL << 60U);
        const std::size_t offset = i * tag_bytes;
        write_u64_le(bytes, offset, tag);
        write_u64_le(bytes, offset + 8U, addresses[i]);
        write_u64_le(bytes, offset + 16U, values[i]);
        write_u64_le(bytes, offset + 24U, 0ULL);
    }
    return bytes;
}
} // namespace

extern "C" int32_t retroshelf_engine_render_demo_frame(void) {
    constexpr std::size_t width = 320;
    constexpr std::size_t height = 240;
    if (!g_preview_framebuffer.resize(width, height)) {
        g_preview_ready = false;
        return 0;
    }
    g_preview_framebuffer.clear({12, 18, 32, 255});

    retroshelf::GsGifPipeline pipeline;
    const auto stream = make_preview_gif_stream();
    const retroshelf::GifPacket packet{stream, 1};
    g_preview_ready = pipeline.consume_and_render({packet}, g_preview_framebuffer);
    return g_preview_ready ? 1 : 0;
}

extern "C" const uint8_t *retroshelf_engine_framebuffer_pixels(void) {
    if (!g_preview_ready) return nullptr;
    return g_preview_framebuffer.rgba_bytes().data();
}

extern "C" uint32_t retroshelf_engine_framebuffer_width(void) {
    return g_preview_ready ? static_cast<uint32_t>(g_preview_framebuffer.width()) : 0U;
}

extern "C" uint32_t retroshelf_engine_framebuffer_height(void) {
    return g_preview_ready ? static_cast<uint32_t>(g_preview_framebuffer.height()) : 0U;
}

extern "C" uint32_t retroshelf_engine_abi_version(void) {
    return 1U;
}

extern "C" const char *retroshelf_engine_status_message(void) {
    return "Development foundation only; BIOS boot and game rendering are not implemented.";
}

extern "C" int32_t retroshelf_engine_can_boot_games(void) {
    return 0;
}

extern "C" int32_t retroshelf_engine_validate_disc_boot_image(const char *path) {
    if (path == nullptr || path[0] == '\0') return 0;

    retroshelf::Iso9660Disc disc;
    std::string error;
    if (!disc.open(std::filesystem::path(path), &error)) return 0;

    std::string executable_path;
    retroshelf::Elf32MipsImage image;
    return retroshelf::load_ps2_boot_image(disc, executable_path, image, &error) ? 1 : 0;
}

extern "C" int32_t retroshelf_engine_run_disc_subset(
    const char *path,
    uint64_t instruction_budget,
    uint64_t *instructions_executed,
    uint64_t *final_pc,
    int32_t *stop_reason) {
    if (instructions_executed) *instructions_executed = 0;
    if (final_pc) *final_pc = 0;
    if (stop_reason) *stop_reason = -1;
    if (path == nullptr || path[0] == '\0') return 0;

    retroshelf::Iso9660Disc disc;
    std::string error;
    if (!disc.open(std::filesystem::path(path), &error)) return 0;

    retroshelf::Ps2EeMemoryMap memory;
    retroshelf::Cpu cpu;
    const std::size_t budget = static_cast<std::size_t>(
        std::min<uint64_t>(instruction_budget,
                           static_cast<uint64_t>(std::numeric_limits<std::size_t>::max())));
    const auto result = retroshelf::load_and_execute_ps2_disc(disc, memory, cpu, budget);
    if (!result.image_loaded) return 0;

    if (instructions_executed)
        *instructions_executed = static_cast<uint64_t>(result.execution.instructions_executed);
    if (final_pc) *final_pc = result.execution.final_pc;
    if (stop_reason) {
        switch (result.execution.reason) {
            case retroshelf::EeExecutionRunner::StopReason::InstructionLimit:
                *stop_reason = 0;
                break;
            case retroshelf::EeExecutionRunner::StopReason::CpuExceptionOrUnsupportedInstruction:
                *stop_reason = 1;
                break;
            case retroshelf::EeExecutionRunner::StopReason::InvalidInstructionFetch:
                *stop_reason = 2;
                break;
        }
    }
    return 1;
}
