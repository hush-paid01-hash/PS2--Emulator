# Original PS2 iPhone Emulator

An independent PlayStation 2 emulator project for iPhone, built around an original emulation core.

## Project direction

**PS2 only. No PS3 support is planned.**

This project is intended to implement its own PS2 hardware emulation. It will not use Play!, PCSX2, or another emulator as its core. The app's identity, frontend, and core are being developed for this project.

## Current status

The current `engine/` code is only an educational MIPS-like CPU prototype. It is not yet a real PS2 emulator and cannot boot or play PS2 games. The project must still implement the PS2 Emotion Engine (R5900), vector units, memory and bus, DMA, timers, interrupts, Graphics Synthesizer, audio, disc I/O, and BIOS boot support.

## Planned features

- Original PS2 emulation core written for this project
- iPhone-focused interface and touch controls
- External controller support
- Import of supported disc images through the iOS Files picker
- User-selected BIOS/firmware files where required
- Save states and memory-card management
- Per-game settings and performance/FPS overlay
- ARM64 optimization, subject to iOS platform restrictions and real-device testing

## Development approach

Core logic will be portable C++ with automated tests and GitHub Actions builds. The iOS frontend will be kept separate from the emulation engine. Large builds should use hosted CI runners rather than relying on a small cloud VM.

Use only BIOS and game images you are legally entitled to use. No copyrighted firmware or game images will be bundled by this project.

## Roadmap

See [the original PS2 core roadmap](docs/ORIGINAL_PS2_CORE_ROADMAP.md) and [the current PS2 engineering plan](docs/REAL_PS2_EMULATOR_PLAN.md).

## License

The project license and third-party notices must be finalized before distribution.
