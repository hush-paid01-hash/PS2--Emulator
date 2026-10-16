#include "retroshelf/engine_c_api.h"

#include <cassert>
#include <cstring>

int main() {
    assert(retroshelf_engine_abi_version() == 1U);
    const char *message = retroshelf_engine_status_message();
    assert(message != nullptr);
    assert(std::strstr(message, "not implemented") != nullptr);
    assert(retroshelf_engine_can_boot_games() == 0);

    // Preflight must fail safely for missing, empty, or null paths.
    assert(retroshelf_engine_validate_disc_boot_image(nullptr) == 0);
    assert(retroshelf_engine_validate_disc_boot_image("") == 0);
    assert(retroshelf_engine_validate_disc_boot_image(
        "this-file-should-not-exist-hush-preflight.iso") == 0);
    assert(retroshelf_engine_inspect_bios_image(nullptr) == 0);
    assert(retroshelf_engine_inspect_bios_image("") == 0);
    assert(retroshelf_engine_inspect_bios_image(
        "this-file-should-not-exist-hush-bios-check.bin") == 0);
    return 0;
}
