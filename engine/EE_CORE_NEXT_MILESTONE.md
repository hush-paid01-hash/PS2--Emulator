# Hush EE Core: next implementation milestone

This document records the next engineering gate for the original Hush PS2 emulator. It is a work plan, not a claim that the features below are implemented.

## Current verified boundary

The current CPU is a limited interpreter. It stores each EE general-purpose register as separate 64-bit low/high halves, and LQ/SQ cover basic transfer of both halves. That storage is not equivalent to complete R5900 128-bit instruction semantics: the MMI instruction families and broader EE behavior remain incomplete. HI/LO and several multiply/divide operations already exist in partial form and still require manual-based semantic auditing. The BIOS image loader maps bytes but does not by itself provide a complete BIOS boot path. The command-line program is a small arithmetic demonstration. Passing unit tests is not evidence of retail-game compatibility.

## Implementation order

1. **Lock down instruction semantics with tests.** Add focused tests for R5900 integer instructions, sign-extension rules, exception state, delay slots, and edge cases. Cross-check each behavior against the EE Core Instruction Set Manual and PS2Tek documentation.
2. **Validate EE architectural state and 128-bit semantics.** Preserve the existing low/high 64-bit GPR storage and verify reset, zero-register, LQ/SQ, and instruction writeback invariants. Audit the existing HI/LO and multiply/divide operations against the EE manual; add missing EE-specific state or behavior only where the reference requires it.
3. **Implement missing base and EE-specific instructions incrementally.** Prioritize the boot-critical integer/COP0 subset first, then MMI instructions. Each instruction group needs regression tests before being used by the execution runner.
4. **Correct exceptions and mapped memory behavior.** Model reset/exception vectors, COP0 status and cause semantics, address translation/aliases, memory-mapped hardware access, and exception return paths with tests.
5. **Connect BIOS boot to real EE execution.** Use a legally obtained BIOS, record a bounded trace, and distinguish real firmware execution from synthetic test images.
6. **Build the minimum graphics path.** Implement GS register writes and the GIF-to-GS path sufficiently to produce a framebuffer; packet parsing alone is not rendering.
7. **Prove game execution before calling it playable.** Test a legally obtained disc image through boot, CPU execution, graphics output, input, and stable frame updates. Keep failures reproducible in automated tests.
8. **Integrate and verify the iPhone app.** Add a real iOS app target, connect the native frontend to the engine, import BIOS/disc files through iOS Files, and validate on a physical iPhone. Signing/install success alone does not prove gameplay.

## Acceptance gates

- Unit tests and CI pass for each change.
- No claim of BIOS boot until a real BIOS has been run and the trace is recorded.
- No claim of playability until a real game reaches interactive gameplay on a physical iPhone build.
- Synthetic BIOS tests, CLI demos, and simulator-only builds are useful tests but do not satisfy the gameplay gate.

## References

- PS2Tek Emotion Engine reference: https://israpps.github.io/ps2tek/PS2/EE/
- PS2 hardware and instruction manuals index: https://github.com/ninjadynamics/PS2Docs
