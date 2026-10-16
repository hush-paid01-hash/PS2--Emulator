#include "retroshelf/engine_c_api.h"

#include "retroshelf/cpu.hpp"
#include "retroshelf/iso9660_disc.hpp"
#include "retroshelf/ps2_disc_execution.hpp"
#include "retroshelf/ps2_ee_memory_map.hpp"
#include "retroshelf/ps2_boot_image.hpp"

#include <algorithm>
#include <filesystem>
#include <limits>
#include <string>


#include "retroshelf/gs_framebuffer.hpp"
#include "retroshelf/gs_software_rasterizer.hpp"

#include <cstddef>
#include <cstdint>

namespace {
retroshelf::GsFramebuffer g_preview_framebuffer;
bool g_preview_ready = false;
}

extern "C" int32_t retroshelf_engine_render_demo_frame(void) {
    constexpr std::size_t width = 320;
    constexpr std::size_t height = 240;
    if (!g_preview_framebuffer.resize(width, height)) {
        g_preview_ready = false;
        return 0;
    }

    g_preview_framebuffer.clear({12, 18, 32, 255});
    retroshelf::GsSoftwareRasterizer rasterizer;
    const bool first = rasterizer.draw_triangle(
        g_preview_framebuffer, {160, 28}, {38, 202}, {282, 202},
        {0, 220, 255, 255});
    const bool second = rasterizer.draw_triangle(
        g_preview_framebuffer, {160, 68}, {104, 158}, {216, 158},
        {255, 90, 120, 255});
    g_preview_ready = first && second;
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
