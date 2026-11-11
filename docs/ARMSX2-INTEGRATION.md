# Hush + ARMSX2 integration

## Pinned upstream dependency

The upstream ARMSX2 repository is tracked as a Git submodule:

- Path: `third_party/ARMSX2`
- Remote: https://github.com/ARMSX2/ARMSX2.git
- Pinned revision: `0f24a1e68571e4db59bdeb592facc3cee019a001`
- License: GPL-3.0 (see `THIRD_PARTY_NOTICES.md` and the submodule's `COPYING.GPLv3`).

The submodule is pinned rather than following a moving branch so builds are reproducible and upstream changes can be reviewed deliberately.

## Build the upstream iOS ARM64 core

On a Mac with full Xcode, CMake 4.4+, Ninja, and the required Xcode iOS platform installed:

```sh
git submodule update --init --recursive
cd third_party/ARMSX2
./platforms/ios/scripts/build-ios-core.sh device
```

The upstream script configures and builds the `armsx2_core_libs` target into `platforms/ios/build/core-device`. For simulator builds, use `sim` instead of `device`.

GitHub Actions runs this core-only build on macOS. A successful core build proves that the pinned upstream ARM64 iOS core compiles; it does not prove that the core is linked to Hush, that the Hush app installs, or that a PS2 game plays.

## Hush application architecture

Hush owns the product experience: its name, visual system, navigation, library presentation, onboarding, and user-facing settings. The emulator implementation comes from the pinned ARMSX2 dependency. Do not copy ARMSX2 branding or present Hush as an official ARMSX2 build.

The existing `ios/OriginalPS2` SwiftUI app and `engine/` C++ library are an earlier, incomplete prototype. They currently expose a limited test engine and are not an ARMSX2 runtime adapter. They must not be represented as playable or silently treated as the ARMSX2 core.

## Remaining integration work

1. Map the upstream iOS runtime and bridge lifecycle to a Hush-owned SwiftUI shell.
2. Replace the prototype engine bridge with an explicit adapter to the upstream iOS runtime; keep upstream implementation files and notices intact.
3. Connect Hush's Files import and security-scoped file access to the upstream BIOS/disc loading APIs.
4. Connect the upstream Metal renderer, audio output, touch/controller input, save-state and memory-card APIs.
5. Keep user settings and library metadata in Hush-owned storage without changing upstream save formats.
6. Build the combined Hush app, then validate BIOS boot and actual games on physical iPhones with JIT availability documented.
7. Before distributing, provide the required GPL-3.0 notices and corresponding source for the covered combined work.

## Release gate

Do not call Hush playable because the upstream core compiles or because the prototype tests pass. A release candidate must demonstrate end-to-end game boot, live graphics, audio, and responsive controls on a physical iPhone. Test multiple titles and record device, iOS version, JIT state, and compatibility results.
