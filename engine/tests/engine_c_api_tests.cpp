#include "retroshelf/engine_c_api.h"

#include <cassert>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <fstream>

int main() {
    assert(retroshelf_engine_abi_version() == 1U);
    const char *message = retroshelf_engine_status_message();
    assert(message != nullptr);
    assert(std::strstr(message, "not implemented") != nullptr);
    assert(retroshelf_engine_can_boot_games() == 0);
    assert(retroshelf_engine_trace_count() == 0U);
    assert(retroshelf_engine_trace_pc(0U) == 0U);
    assert(retroshelf_engine_trace_instruction(0U) == 0U);
    assert(retroshelf_engine_trace_fetched(0U) == -1);

    // The exported framebuffer preview should be a bounded RGBA test frame.
    assert(retroshelf_engine_framebuffer_pixels() == nullptr);
    assert(retroshelf_engine_framebuffer_width() == 0U);
    assert(retroshelf_engine_framebuffer_height() == 0U);
    assert(retroshelf_engine_render_demo_frame() == 1);
    assert(retroshelf_engine_framebuffer_width() == 320U);
    assert(retroshelf_engine_framebuffer_height() == 240U);
    const uint8_t *pixels = retroshelf_engine_framebuffer_pixels();
    assert(pixels != nullptr);
    const uint32_t sample = (40U * 320U + 160U) * 4U;
    assert(pixels[sample] == 0U);
    assert(pixels[sample + 1U] == 220U);
    assert(pixels[sample + 2U] == 255U);
    assert(pixels[sample + 3U] == 255U);

    // BIOS inspection distinguishes readable files from plausible-size files.
    uint64_t biosSize = 123;
    int32_t plausibleBIOS = 123;
    assert(retroshelf_engine_inspect_bios_image(nullptr, &biosSize, &plausibleBIOS) == 0);
    assert(biosSize == 0 && plausibleBIOS == 0);
    const auto biosDir = std::filesystem::temp_directory_path() / "hush_c_api_bios_test";
    std::filesystem::create_directories(biosDir);
    const auto tinyBIOS = biosDir / "tiny-bios.bin";
    { std::ofstream out(tinyBIOS, std::ios::binary); out.put('\0'); }
    biosSize = 0;
    plausibleBIOS = 1;
    assert(retroshelf_engine_inspect_bios_image(tinyBIOS.string().c_str(), &biosSize, &plausibleBIOS) == 1);
    assert(biosSize == 1 && plausibleBIOS == 0);
    const auto plausibleBIOSPath = biosDir / "plausible-bios.bin";
    { std::ofstream out(plausibleBIOSPath, std::ios::binary); out.seekp(4 * 1024 * 1024 - 1); out.put('\0'); }
    assert(retroshelf_engine_inspect_bios_image(plausibleBIOSPath.string().c_str(), &biosSize, &plausibleBIOS) == 1);
    assert(biosSize == 4ULL * 1024 * 1024 && plausibleBIOS == 1);
    assert(retroshelf_engine_inspect_bios_image((biosDir / "missing.bin").string().c_str(), &biosSize, &plausibleBIOS) == 0);
    assert(biosSize == 0 && plausibleBIOS == 0);
    // Execute two synthetic MIPS instructions from the mapped BIOS reset vector.
    const auto resetBios = biosDir / "reset-vector-bios.bin";
    {
        std::ofstream out(resetBios, std::ios::binary);
        const std::uint32_t words[] = {
            (0x09U << 26U) | (8U << 16U) | 7U, // ADDIU t0,zero,7
            (0x09U << 26U) | (9U << 16U) | 5U  // ADDIU t1,zero,5
        };
        for (const std::uint32_t word : words) {
            out.put(static_cast<char>(word & 0xFFU));
            out.put(static_cast<char>((word >> 8U) & 0xFFU));
            out.put(static_cast<char>((word >> 16U) & 0xFFU));
            out.put(static_cast<char>((word >> 24U) & 0xFFU));
        }
    }
    uint64_t biosInstructions = 99;
    uint64_t biosPC = 99;
    int32_t biosReason = 99;
    assert(retroshelf_engine_run_bios_reset_vector(
        resetBios.string().c_str(), 2, &biosInstructions, &biosPC, &biosReason) == 1);
    assert(biosInstructions == 2);
    assert(biosPC == 0xBFC00008ULL);
    assert(biosReason == 0);
    biosInstructions = 99;
    biosPC = 99;
    biosReason = 99;
    assert(retroshelf_engine_run_bios_reset_vector(
        nullptr, 10, &biosInstructions, &biosPC, &biosReason) == 0);
    assert(biosInstructions == 0 && biosPC == 0 && biosReason == -1);

    std::filesystem::remove_all(biosDir);

    // Preflight must fail safely for missing, empty, or null paths.
    assert(retroshelf_engine_validate_disc_boot_image(nullptr) == 0);
    assert(retroshelf_engine_validate_disc_boot_image("") == 0);
    assert(retroshelf_engine_validate_disc_boot_image(
        "this-file-should-not-exist-hush-preflight.iso") == 0);

    // The bounded-run API initializes outputs and rejects invalid paths.
    uint64_t instructions = 99;
    uint64_t pc = 99;
    int32_t reason = 99;
    assert(retroshelf_engine_run_disc_subset(nullptr, 100, &instructions, &pc, &reason) == 0);
    assert(instructions == 0);
    assert(pc == 0);
    assert(reason == -1);
    assert(retroshelf_engine_trace_count() == 0U);

    instructions = 99;
    pc = 99;
    reason = 99;
    assert(retroshelf_engine_run_disc_subset(
        "this-file-should-not-exist-hush-run.iso", 100, &instructions, &pc, &reason) == 0);
    assert(instructions == 0);
    assert(pc == 0);
    assert(reason == -1);

    // Output pointers are optional; failure remains safe when they are null.
    assert(retroshelf_engine_run_disc_subset("", 0, nullptr, nullptr, nullptr) == 0);
    return 0;
}
