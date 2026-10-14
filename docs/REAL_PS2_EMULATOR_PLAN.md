# Path to a Real Playable PS2 Emulator on iPhone

## Current status

The `engine/` folder in this repository is an educational CPU prototype. It is **not** a PlayStation 2 emulator and cannot boot commercial PS2 games. Do not describe it as playable.

## Recommended technical direction

Integrate a maintained, real PS2 emulator core instead of attempting to recreate the PS2 hardware from the current small CPU prototype. The first candidate to evaluate is the upstream **Play!** project:

- Upstream source: https://github.com/jpd002/Play-
- Upstream project describes support for iOS and other platforms.
- Play! has a built-in high-level BIOS approach; review its current license and upstream instructions before redistributing or modifying it.
- Its iOS documentation warns that JIT is not enabled by default and game launch can fail without JIT. Current iOS/iPhone compatibility and permitted JIT-enablement paths must be tested on the actual target device and iOS version.

PCSX2 is another well-known real PS2 emulator, but its official project targets Windows, Linux, and macOS; it is not a drop-in iOS core. Do not assume desktop PCSX2 can simply be compiled into an iPhone app.

## Integration milestones

1. **Preserve the prototype:** keep `engine/` as a learning/test harness. Do not label it as a functioning PS2 core.
2. **Review the core and license:** inspect upstream Play! source, its license, iOS installer/build files, dependency list, and current activity. Keep required copyright and license notices. Do not copy a large third-party core without preserving its license.
3. **Create an iOS application target:** use an Apple build/signing environment (normally macOS with Xcode) to produce and sign an iPhone app. A GitHub repository or Ubuntu server alone cannot produce a signed, installed iOS app.
4. **Connect the frontend:** add a game library, iOS Files import for user-owned disc images, touch controls, controller support, per-game settings, save-state UI, memory-card management where supported by the core, and an FPS/performance overlay.
5. **Handle JIT honestly:** test what is permitted and works on the exact iOS version and device. Do not promise full-speed performance or bypass platform protections. Provide a documented non-JIT fallback only if the selected core supports it.
6. **Test with lawful material:** use homebrew/test images and game dumps the user is authorized to use. Do not bundle copyrighted game images or console firmware.
7. **Release gate:** do not call it playable until a signed build launches, imports a test image, boots it, renders frames, accepts input, saves/loads state if supported, and passes a documented compatibility test.

## Current blockers

- No iOS/Xcode build and signing run has been demonstrated for this repository.
- No upstream emulator core has yet been integrated into this repository.
- No real PS2 game has been booted or tested by this project.
- iOS JIT availability and distribution route must be verified on the actual target device.

## Definition of done

A real playable release means a working signed iPhone build and verified game boot—not just compiling a CPU library or passing unit tests.