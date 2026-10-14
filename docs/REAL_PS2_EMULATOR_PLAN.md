# Roadmap to a Playable Original PS2 Emulator on iPhone

## Non-negotiable project scope

This project aims to implement its own PlayStation 2 emulation core. Do not substitute Play!, PCSX2, or another emulator core. PS3 support is out of scope. This is a substantial hardware-emulation project; passing prototype tests does not mean it can boot a PS2 game.

## Current verified foundation

- A SwiftUI iPhone front-end shell with BIOS and disc file pickers.
- GitHub Actions can generate an Xcode project and build the app for the iOS Simulator.
- A portable C++ core with an early EE/R5900 instruction subset, 2 MiB test RAM, a machine fetch/execute harness, and prototype timer/interrupt/DMA structures.
- Automated tests run on GitHub-hosted Linux/macOS runners.

The file picker currently copies selected files into app storage, but the engine does not yet parse a BIOS, mount a disc image, or boot a game. The current CPU and hardware models are incomplete and are not cycle-accurate PS2 hardware.

## Implementation order

1. **Stabilize the EE interpreter.** Define instruction semantics and exception behavior; expand arithmetic, branch/delay-slot, load/store, COP0, and privileged instruction support; add instruction-by-instruction regression tests.
2. **Implement a memory subsystem.** Add verified address translation, RAM/ROM regions, alignment and bus faults, BIOS mapping, and memory-mapped device dispatch. Replace placeholder addresses with documented PS2 hardware addresses.
3. **Boot firmware.** Add a BIOS image loader that validates size and mapping, initializes the correct reset state, and traces execution with useful diagnostics. Only support firmware supplied by the user and legally obtained.
4. **Implement PS2 hardware in dependency order.** Interrupt controller and timers, DMA channels and arbitration, scratchpad and cache behavior, VU0/VU1 and micro-instructions, GIF path, GS rendering, and SPU2 audio. Keep every device independently testable.
5. **Implement disc support.** Parse supported ISO formats and the PS2 disc filesystem; load data through the emulated I/O path. Do not treat file selection as disc emulation.
6. **Connect the iOS front end to the native C++ core.** Use a C-compatible bridge, copy imported files into sandboxed app storage, validate them, and surface errors. Add touch/controller input, save states, memory cards, per-game settings, and performance overlays only as their backend features become real.
7. **Optimize after correctness.** Keep a portable interpreter as the baseline. Consider ARM64 optimizations only after conformance tests pass. Do not promise a native ARM64 JIT or full speed until it is implemented and tested on the target iOS version.
8. **Release gate.** Produce a signed iPhone build through a legitimate Apple signing route; test on the actual iPhone; boot lawful homebrew/test software; verify graphics, sound, input, and persistence. Do not call the app playable before this gate passes.

## Build environment and constraints

Use GitHub-hosted Actions runners for builds; a small cloud VM is not a substitute for Xcode and is not expected to handle the full project. A successful iOS Simulator build is not a signed iPhone install. Native device distribution requires an appropriate signing/provisioning path. JIT availability and app-distribution rules must be checked against current Apple policy and the actual device.

## Legal materials

The app must not bundle copyrighted BIOS firmware or commercial game images. Users should provide only firmware and game dumps they are legally entitled to use.

## Honest definition of done

The project is playable only when a signed iPhone build can load a user-provided compatible BIOS and disc image, boot a PS2 program, render frames, accept input, and pass documented compatibility tests. A green C++ test run or an iOS Simulator build alone does not satisfy this definition.
