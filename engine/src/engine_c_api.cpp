#include "retroshelf/engine_c_api.h"

extern "C" uint32_t retroshelf_engine_abi_version(void) {
    return 1U;
}

extern "C" const char *retroshelf_engine_status_message(void) {
    return "Development foundation only; BIOS boot and game rendering are not implemented.";
}

extern "C" int32_t retroshelf_engine_can_boot_games(void) {
    return 0;
}
