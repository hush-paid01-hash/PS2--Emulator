#ifndef RETROSHELF_ENGINE_C_API_H
#define RETROSHELF_ENGINE_C_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Stable C ABI for native frontends such as Swift.
 *
 * This API reports the current development engine's capabilities only. It
 * deliberately does not claim BIOS boot, game execution, graphics, audio, or
 * controller support, because those systems are not implemented end-to-end.
 */
uint32_t retroshelf_engine_abi_version(void);
const char *retroshelf_engine_status_message(void);
int32_t retroshelf_engine_can_boot_games(void);

#ifdef __cplusplus
}
#endif

#endif
