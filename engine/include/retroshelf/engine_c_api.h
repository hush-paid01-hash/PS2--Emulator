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
 * deliberately does not claim BIOS boot, full game execution, graphics, audio,
 * or controller support, because those systems are not implemented end-to-end.
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
 * Development-only execution smoke test. Loads the disc's SYSTEM.CNF ELF into
 * the current limited EE main-RAM model, then runs at most instruction_budget
 * instructions using the implemented CPU subset. It does NOT boot the BIOS or
 * provide complete PS2 hardware emulation, graphics, audio, or playability.
 *
 * Returns 1 if the ELF was loaded and the bounded runner started, or 0 if
 * arguments/disc/ELF loading failed. Output pointers may be NULL.
 * stop_reason: 0 = instruction budget reached, 1 = CPU exception or unsupported
 * instruction, 2 = invalid instruction fetch.
 */
int32_t retroshelf_engine_run_disc_subset(const char *path,
                                          uint64_t instruction_budget,
                                          uint64_t *instructions_executed,
                                          uint64_t *final_pc,
                                          int32_t *stop_reason);

#ifdef __cplusplus
}
#endif

#endif
