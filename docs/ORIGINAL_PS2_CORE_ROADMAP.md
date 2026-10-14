# Original PS2 Core: Engineering Roadmap

This project will implement its own PlayStation 2 emulation core. It will not embed, wrap, or depend on Play!, PCSX2, or another emulator core.

## Scope and honest status

The existing `engine/` code is an educational CPU prototype, not a functional PS2 emulator. This roadmap defines the original implementation path. Compatibility with commercial games is a long-term goal, not a current capability.

## Architecture

- **EE CPU:** MIPS III / R5900-compatible instruction decoding and execution, 128-bit GPR behavior, COP0/system control, exceptions, branch-delay behavior, and cycle accounting.
- **Memory and bus:** physical address map, RAM/ROM regions, scratchpad, memory-mapped devices, and access-width rules.
- **VU0 and VU1:** vector instruction execution, vector registers, flags, micro-memory, and microprogram control.
- **DMAC and interrupts:** channel state, transfer modes, priority/arbitration, and interrupt delivery.
- **Timers and event scheduler:** deterministic cycle-based device events.
- **Graphics Synthesizer:** register model, local video memory, transfer paths, and a staged renderer.
- **Audio and I/O:** sound hardware model, controller ports, memory cards, and disc-drive interfaces.
- **Boot and media:** user-selected BIOS and game images through the iOS Files picker. Users must provide firmware and game dumps they are legally entitled to use.
- **Frontend:** native iOS UI, touch and controller input, per-game profiles, save states, and performance overlay. The frontend must call only this project's core API.

## Build order

1. Establish a portable C++ core library and deterministic test runner.
2. Replace the toy CPU with a documented R5900 instruction decoder and execution loop.
3. Add unit tests for arithmetic, 64-bit and 128-bit register semantics, branches, exceptions, and edge cases.
4. Implement memory mapping and cycle accounting.
5. Add device interfaces and the event scheduler before implementing DMA, timers, interrupts, VU units, GS, audio, and disc I/O.
6. Add trace logs and reproducible synthetic tests; test with original homebrew test programs where possible.
7. Integrate the independently written core with an iOS frontend and test on real ARM64 hardware.
8. Add save-state serialization only after CPU and device state layouts stabilize.

## Engineering rules

- No external emulator core as a runtime dependency.
- Public hardware documentation and clean-room tests may inform behavior; do not copy implementation code from other emulator projects.
- Keep each device behind a small interface so it can be tested independently.
- Prefer portable C++ for core logic; keep iOS UI and platform integration separate.
- Use GitHub-hosted CI runners for builds and tests where possible. Do not assume the small AWS VM has enough RAM or disk for large builds.
- A real native iOS release still requires an Apple-platform build/signing path. iOS JIT execution is constrained; do not promise a JIT or performance level until tested on-device.

## First milestone: EE CPU foundation

The next code milestone is a testable R5900 core skeleton with explicit register state, instruction fetch/decode/execute separation, PC and delay-slot handling, and a test harness. It must pass synthetic instruction tests before any claim of PS2 boot support.

## Definition of done for the first milestone

- Clean CMake configure and build on a hosted CI runner.
- Automated tests cover each implemented instruction and its edge cases.
- Unsupported instructions fail predictably and produce useful diagnostics.
- No Play!, PCSX2, or other emulator implementation is linked or copied into this project.
