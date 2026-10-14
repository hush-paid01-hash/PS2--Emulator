# iPhone frontend plan (PS2 only)

This directory is the planned native iOS frontend for the independently developed PS2 core. No third-party emulator core is used.

## Frontend responsibilities

- SwiftUI library and game details screens
- FileImporter for user-selected ISO, BIN, IMG, CHD and BIOS files (support must be verified by the eventual disc parser)
- Controller/touch input mapping
- Per-game configuration and performance overlay
- Save-state and memory-card file management
- A narrow C/C++ bridge to the original `retroshelf-ee-core` library

## Current status

This is a design/implementation boundary, not an installable app yet. The repository does not currently contain a complete Xcode project, Apple signing setup, a disc parser, a functioning PS2 device model, or a bootable emulator. A SwiftUI screen alone cannot play games until the core, graphics/audio, BIOS boot and platform bridge exist.

## Build and signing

Use a macOS GitHub Actions runner with Xcode for native iOS builds. Running the C++ unit tests on Ubuntu does not create an iPhone app. Installation on a physical iPhone requires an appropriate Apple signing and distribution route. iOS JIT limitations mean the first version should use an interpreter; any later ARM64 JIT needs a separate platform-feasibility review.

## File handling

Use Apple's document picker so the user selects their own files. Do not bundle BIOS firmware or commercial games. Persist bookmarks or copy user-selected files into app-managed storage with clear user consent. Validate format headers and sizes before attempting to load an image.
