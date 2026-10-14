# Original PS2 iPhone Emulator

A PS2-only project aiming to build an independently implemented PlayStation 2 emulation core for iPhone.

## Core rule

**No PS3 support. No Play!, no PCSX2, and no other existing emulator core.** This project is building its own CPU and hardware emulation. The project name has not yet been changed, but the intended scope is PS2 only.

## Current engineering status

The C++ project contains:
- An educational MIPS-like CPU prototype (legacy demonstration)
- An early R5900/Emotion Engine CPU instruction subset
- A basic 2 MiB RAM/bus model for isolated tests
- An EE machine harness that fetches instruction words from that RAM and passes them to the CPU
- CMake and GitHub Actions test targets

**This is not yet a functional PS2 emulator.** The instruction set and hardware model are incomplete; there is no BIOS boot, full memory map, DMA, interrupts, VU0/VU1, Graphics Synthesizer renderer, SPU2 audio, disc-drive emulation, native iOS app, or playable game support yet.

## Planned features

- Original EE/R5900 CPU, vector units and PS2 device models
- Native iPhone interface, touch controls and external controllers
- User-selected BIOS and disc images through Files
- Save states and memory cards
- Per-game settings and performance/FPS overlay
- Portable C++ core, automated tests and ARM64 optimization where supported

## Development and legality

Use GitHub Actions hosted runners for C++ builds; the small AWS VM is not assumed to have enough resources for large builds. Native iOS builds require an Apple-platform build/signing path. JIT execution is restricted on iOS, so interpreter-first development is the realistic initial route.

Use only BIOS and game images you are legally entitled to use. Firmware and games will not be bundled.

## Roadmap

- [Original core roadmap](docs/ORIGINAL_PS2_CORE_ROADMAP.md)
- [iOS frontend plan](ios/README.md)
