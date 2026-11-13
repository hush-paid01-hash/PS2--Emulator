# Native iOS front-end scaffold

This directory contains an early SwiftUI shell for the RetroShelf PS2 project.

## Included
- iOS Files pickers for ISO, BIN, IMG, CHD game images and a user-supplied BIOS file.
- Copies selected files into the app's Application Support/Imported directory.
- Basic touch-control layout and GameController discovery UI.
- Explicit status messages so the shell does not falsely claim to boot a game.

## Important: not playable yet
This is **not a working PS2 emulator**. The current C++ engine is an incomplete hardware/CPU foundation. The Swift shell is not yet linked to the C++ core; touch controls are deliberately disabled; controller input is detected but not mapped into emulated buttons. BIOS selection only imports a file—it does not execute it. ISO/BIN/IMG/CHD selection does not mount or parse the disc image. No Xcode project or signed app build is included, and no physical-iPhone test has been performed.

## Integration work required
1. Add this source to a real iOS Xcode target and configure signing/capabilities.
2. Expose the C++ engine through a C-compatible bridge and link it into the app.
3. Implement PS2 BIOS boot, EE/IOP CPUs, memory map, interrupts, timers, VIF/GIF/DMA, GS rendering, CD/DVD image reading, and SPU2/IOP behavior.
4. Implement disc filesystem/executable loading, save states, memory cards, audio, frame pacing, and input mapping.
5. Test on a supported physical iPhone with a legally obtained BIOS and game image.

Do not treat the presence of this shell as evidence that a game can run.
