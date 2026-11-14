# Hush EE Core: next implementation milestone

This document records the next engineering gates for the original Hush PS2 emulator. It is a work plan, not a claim that the features below are complete or that Hush is playable.

## Current implemented foundation

The repository now has several connected development components:

- A limited EE interpreter with a subset of integer, COP0, memory, and EE-specific instructions. GPR storage has low/high 64-bit halves, but that does not mean complete R5900/MMI semantics.
- An EE memory map for main-RAM aliases, a read-only BIOS ROM window, and scratchpad. TLB behavior and complete reset state are not implemented.
- A bounded BIOS reset-vector runner. It maps EE timer and INTC MMIO devices and charges one approximate cycle per attempted instruction. This is a diagnostic execution harness, not proof that a real BIOS completes boot.
- Four EE timer register blocks and an INTC status/mask register block. Timer event timing is deliberately approximate; external H/V-blank sources and accurate CPU cycle accounting are not modeled.
- A GIF DMA transfer engine with normal and chain modes, connected by a reusable runtime to GIF packet parsing, GS state, local memory, and software framebuffer rendering. This runtime can render test streams, but the GIF DMA registers are not yet exposed through an EE DMAC MMIO controller for guest CPU programming.
- Disc/ELF inspection and a bounded ELF execution subset. Loading an ELF and executing a small instruction subset is not equivalent to booting a retail PS2 game.

The native preview framebuffer is a synthetic GIF test pattern, not output from a running game. The C API correctly reports that game boot is not yet supported. Unit tests, synthetic streams, and CI builds are not evidence of retail-game compatibility.

## Next implementation order

1. **Verify the outstanding CPU regression fix.** PR #296 corrects PEXT5 to read two packed halfwords from each 64-bit half of a 128-bit GPR. Its CI result must be inspected before merge; do not infer success from an empty status response.
2. **Add an EE DMAC MMIO controller, starting with GIF channel 2.** Implement the verified channel/global register map, guest-visible CHCR/MADR/QWC/TADR behavior, start/stop semantics, bounded transfer scheduling, D_STAT interrupt status, and propagation to EE INTC. Start with the GIF path only and label unsupported channels/registers explicitly.
3. **Connect guest CPU writes to GIF rendering.** Map the DMAC device into the same SystemBus used by the EE runner. A guest program should configure GIF channel 2 and trigger a transfer; tests should assert the actual GIF parser/GS pipeline receives the payload, DMA completion state is visible, and the corresponding interrupt is raised. This is the first real CPU-to-DMA-to-GS integration gate.
4. **Improve EE timing and interrupt accuracy.** Replace one-cycle-per-instruction approximation with instruction cycle metadata and model timer/DMAC event timing and interrupt masking/acknowledgement accurately enough for boot-critical paths.
5. **Expand EE CPU and COP0 coverage.** Prioritize instructions and exception/ERET behavior required by real BIOS traces. Add regression tests for delay slots, exceptions, TLB-related behavior, and 128-bit MMI operations. Cross-check semantics against the EE manual.
6. **Implement the remaining boot-critical platform path.** Add the IOP/SIF and DMAC channels required by BIOS and game startup, memory-mapped device behavior, and the missing reset/kernel services. Do not substitute host-side shortcuts for guest-visible hardware behavior without documenting them.
7. **Validate real BIOS boot.** Run a legally obtained BIOS image through the guest reset vector, capture a bounded trace, and show concrete firmware progress beyond a reset-vector smoke test.
8. **Reach actual game execution.** Boot a legally obtained PS2 disc image through firmware and guest code, render game-generated GS output, and support input and stable frame updates.
9. **Integrate and verify the iPhone app.** Connect the native engine to a real iOS app target, support BIOS/disc imports through iOS Files, and validate on a physical iPhone. Signing or simulator success alone does not prove gameplay.

## Acceptance gates

- Every hardware milestone has focused regression tests and passing required CI checks.
- No claim of BIOS boot until a real BIOS has demonstrably progressed through firmware execution.
- No claim of playability until a real game reaches interactive gameplay on a physical iPhone build using a genuine BIOS and disc image.
- Synthetic BIOS tests, synthetic GIF streams, CLI demos, and simulator-only builds are useful development checks but do not satisfy the gameplay gate.

## References

- PS2Tek Emotion Engine reference: https://israpps.github.io/ps2tek/PS2/EE/
- PS2 hardware and instruction manuals index: https://github.com/ninjadynamics/PS2Docs
