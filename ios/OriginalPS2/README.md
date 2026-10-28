# Original PS2 for iPhone — native app milestone

This folder contains the project's native iOS frontend. It must call this repository's `engine/` implementation directly; it must not embed or launch another emulator core.

## Current status

The repository now has an XcodeGen project specification at `project.yml`. It defines an iOS app target (`OriginalPS2`) and a static C++ library target (`retroshelf_engine`), with the app depending on and linking the library. The `ios-app-build.yml` workflow generates the Xcode project and attempts an iOS Simulator build, including native linking.

The C++ engine remains an incomplete development foundation. It is not yet capable of booting and playing retail PS2 games. A successful simulator build would prove app compilation and symbol linking only—not game compatibility or device installation.

## Build locally on a Mac

1. Install Xcode and Xcode command-line tools.
2. Install XcodeGen (`brew install xcodegen`).
3. From the repository root, run `xcodegen generate --spec project.yml`.
4. Build with `xcodebuild -project OriginalPS2.xcodeproj -scheme OriginalPS2 -sdk iphonesimulator -destination 'generic/platform=iOS Simulator' CODE_SIGNING_ALLOWED=NO build`.

GitHub Actions runs the same generation/build sequence. The generated `.xcodeproj` is not committed; `project.yml` is the source of truth.

## Engine bridge and limitations

- `EngineBridge.swift` calls the C ABI symbols declared in `engine/include/retroshelf/engine_c_api.h`.
- The app target links the `retroshelf_engine` static library. The CI build should catch unresolved C ABI symbols.
- The current C API deliberately reports that game boot is unsupported. Do not enable a Play button or claim game compatibility based on the status query alone.
- The engine still lacks substantial Emotion Engine, VU, DMA, GS, BIOS boot, and audio functionality.

## Current frontend

- Import a legally obtained PS2 BIOS through the iOS Files picker.
- Import ISO, BIN, IMG, CHD, and ROM files through Files.
- Display clear readiness and error states.
- Connect controls and rendering only after the repository's own core can run them.

Do not describe the app as playable until end-to-end game boot and rendering are demonstrated by tests on a simulator or real device.
