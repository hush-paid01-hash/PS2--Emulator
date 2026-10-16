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

/*
 * Returns 1 when path opens as a supported ISO-9660/BIN/IMG image and its
 * SYSTEM.CNF executable parses as ELF32 MIPS with loadable segments.
 * Returns 0 on any failure. This is a preflight check only: it does not run
 * the executable, verify a BIOS, support CHD decoding, or imply playability.
 */
int32_t retroshelf_engine_validate_disc_boot_image(const char *path);

/*
 * BIOS file sanity check: 0 = invalid/unreadable, 1 = readable and 1-16 MiB,
 * 2 = readable but outside that size range. This does not authenticate,
 * verify compatibility, or load/execute the BIOS.
 */
int32_t retroshelf_engine_inspect_bios_image(const char *path);

#ifdef __cplusplus
}
#endif

#endif
