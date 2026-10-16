# Original PS2 for iPhone — native app milestone

This folder is reserved for the project's own native iOS frontend. It must call this repository's `engine/` implementation directly; it must not embed or launch another emulator core.

## Current status

The C++ engine is an incomplete development foundation. It is not yet capable of booting and playing retail PS2 games. The SwiftUI shell now has a small `EngineBridge.swift` wrapper for the C ABI and displays the engine's current capability message.

The bridge is not yet a complete app integration: this repository still needs an Xcode iOS app target that compiles the Swift files, links the `retroshelf_engine` static library for the correct iOS target, and verifies the ABI calls in a real build. The current GitHub Actions Swift check only parses syntax; it does not link the app. The engine also lacks substantial Emotion Engine, VU, DMA, GS, and audio functionality.

## Bridge integration notes

- `EngineBridge.swift` calls the exported C ABI symbols declared in `engine/include/retroshelf/engine_c_api.h`.
- The app target must link the `retroshelf_engine` library; otherwise these symbols will be unresolved at link time.
- The current C API deliberately reports that game boot is unsupported. Do not enable a Play button or claim game compatibility based on the status query alone.
- Prefer importing the C declarations through the bridging header once the Xcode target is configured, or keep the Swift declarations synchronized with the C header if the direct-symbol wrapper remains in use.

## Planned iPhone interface

- Import a legally obtained PS2 BIOS through the iOS Files picker.
- Import supported disc images through Files.
- Display clear readiness and error states.
- Connect controls and rendering only after the repository's own core can run them.

Do not describe the app as playable until end-to-end game boot and rendering are demonstrated by tests on a real device or simulator.
