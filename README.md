# RetroShelf — Original PS2 Emulator for iPhone

This repository is for an original PlayStation 2 emulator core and an iPhone frontend. **PS3 is out of scope. Play!, PCSX2, and other existing emulator cores are not used as this project's core.**

## Honest status

The project is in early development. The C++ engine has a limited EE/R5900 instruction interpreter, test RAM, a fetch/execute harness, and prototype hardware components. GitHub Actions has successfully built the SwiftUI shell for the iOS Simulator. These milestones are not a playable emulator.

The app shell can present BIOS and game-image file pickers and copy selected files into app storage. It does not yet validate a BIOS, mount a PS2 disc, run a complete PS2 CPU, render PS2 graphics, or produce sound. No signed installable iPhone package or verified game boot has been produced.

## Current features in progress

- Independent C++ EE/R5900 instruction interpreter and regression tests
- Test RAM and CPU fetch/execute machine harness
- Prototype timers, interrupts, scheduler, DMA and system-bus components
- SwiftUI iPhone shell with Files pickers and touch-control preview
- GitHub Actions Linux C++ tests and macOS iOS Simulator build

## Planned work

1. Expand and validate CPU instructions, exceptions, COP0, and memory operations.
2. Implement a documented PS2 memory map, BIOS mapping, and correct boot state.
3. Implement the PS2 vector units, DMA, GIF, Graphics Synthesizer, sound, and disc I/O.
4. Connect the native core to the iOS app through a stable bridge.
5. Implement functional touch/controller input, save states, memory cards, settings, and FPS reporting.
6. Test a signed build on a physical iPhone using lawful homebrew/test images.

## Build

C++ engine:

```sh
cmake -S engine -B engine/build
cmake --build engine/build --parallel 1
ctest --test-dir engine/build --output-on-failure
```

The hosted GitHub Actions workflow runs the portable C++ build and tests. The iOS workflow generates an Xcode project from `ios/project.yml` and builds for the iOS Simulator. A Simulator build is not an installable device app; physical-device deployment requires appropriate Apple signing and provisioning.

## Legal and platform notes

Provide only BIOS and game images you are legally entitled to use; none are bundled. iOS has restrictions on JIT execution, so an interpreter-first approach is used until performance and platform options are verified on the actual device.

See [the original-core roadmap](docs/REAL_PS2_EMULATOR_PLAN.md) and [the iOS project specification](ios/project.yml).
