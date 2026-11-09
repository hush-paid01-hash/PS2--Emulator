# Original PS2 for iPhone — native app milestone

This folder contains the project's native iOS frontend. It must call this repository's `engine/` implementation directly; it must not embed or launch another emulator core.

## Current status

The repository has an XcodeGen project specification at `project.yml`. It defines an iOS app target (`OriginalPS2`) and a static C++ library target (`retroshelf_engine`), with the app depending on and linking the library. The generated Xcode project is named `Hush.xcodeproj`; the app scheme is `OriginalPS2`.

The `ios-app-build.yml` workflow generates the Xcode project, builds and tests the native engine, then builds both iOS Simulator and unsigned iOS-device app targets. A successful build proves compilation and linking only—not game compatibility, signing, installation, or gameplay.

The C++ engine remains an incomplete development foundation. It is not yet capable of booting and playing retail PS2 games.

## Build locally on a Mac

1. Install Xcode and Xcode command-line tools.
2. Install XcodeGen (`brew install xcodegen`).
3. From the repository root, run `xcodegen generate --spec project.yml`.
4. Build the simulator app with:

   `xcodebuild -project Hush.xcodeproj -scheme OriginalPS2 -sdk iphonesimulator -destination 'generic/platform=iOS Simulator' CODE_SIGNING_ALLOWED=NO build`

5. For an unsigned device build, use `-sdk iphoneos -destination 'generic/platform=iOS'` instead. This output is not installable until it is signed and provisioned.

GitHub Actions runs the generation/build sequence on a macOS runner. The generated `.xcodeproj` is not committed; `project.yml` is the source of truth.

## Engine bridge and limitations

- `EngineBridge.swift` calls the C ABI symbols declared in `engine/include/retroshelf/engine_c_api.h`.
- The app target links the `retroshelf_engine` static library. The CI build catches unresolved C ABI symbols.
- The current C API deliberately reports that game boot is unsupported. Do not enable a Play button or claim game compatibility based on the status query alone.
- The engine still lacks substantial Emotion Engine, VU, DMA, GS, BIOS boot, and audio functionality.

## Current frontend

- Import a legally obtained PS2 BIOS through the iOS Files picker.
- Import ISO, BIN, IMG, CHD, and ROM files through Files.
- Display clear readiness and error states.
- Connect controls and rendering only after the repository's own core can run them.

Do not describe the app as playable until end-to-end game boot and rendering are demonstrated by tests on a physical iPhone.
