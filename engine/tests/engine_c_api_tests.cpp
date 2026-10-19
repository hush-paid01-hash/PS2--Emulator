#include "retroshelf/engine_c_api.h"

#include <cassert>
#include <cstring>
#include <cstdint>

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
