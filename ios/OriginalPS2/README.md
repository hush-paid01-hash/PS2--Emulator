# Original PS2 for iPhone — native app milestone

This folder is reserved for the project's own native iOS frontend. It must call this repository's `engine/` implementation directly; it must not embed or launch another emulator core.

## Current status

The C++ engine is an incomplete development foundation. It is not yet capable of booting and playing retail PS2 games. A native SwiftUI interface, a working C++ bridge, an iOS app target, and substantial missing Emotion Engine, VU, DMA, GS, and audio implementation are still required.

## Planned iPhone interface

- Import a legally obtained PS2 BIOS through the iOS Files picker.
- Import supported disc images through Files.
- Display clear readiness and error states.
- Connect controls and rendering only after the repository's own core can run them.

Do not describe the app as playable until end-to-end game boot and rendering are demonstrated by tests on a real device or simulator.