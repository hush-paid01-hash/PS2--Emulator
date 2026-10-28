# RetroShelf Engine Foundation

This folder is the first portable C++ milestone for the original PS2-emulator project.

## Current scope

- A tiny instruction interpreter for a limited MIPS-style subset.
- General-purpose register state and a simple program counter.
- An EE GIF DMA channel model with normal transfers and DMA chain tags.
- A bounds-checked system-bus interface over the current EE RAM allocation.
- A stateful GIF-tag packet parser for PACKED, REGLIST, IMAGE, and IMAGE2 streams.
- A partial GS pipeline with register-state decoding, local-memory/image-upload foundations, and a software framebuffer/rasterizer for supported primitive paths.
- A synthetic GIF-to-framebuffer demo used to check the graphics path, plus automated tests.
- CMake build configuration for Linux and later integration into a larger project.

## Build and test on Ubuntu

Install the small build tools if they are not already available:

```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake
```

Then, from this `engine` folder:

```sh
cmake -S . -B build
cmake --build build -j1
ctest --test-dir build --output-on-failure
./build/retroshelf-engine
```

Using `-j1` keeps memory use lower on small cloud servers.

## Native frontend C ABI (scaffolding)

The engine sources are also built as the reusable `retroshelf_engine` static
library. `include/retroshelf/engine_c_api.h` exposes a small C ABI that Swift
or another native frontend can import without exposing C++ types. The current
API reports the ABI version and an explicit capability status; it returns
`can_boot_games = 0` because a complete BIOS-to-rendered-game path does not
exist yet.

The file `ios/OriginalPS2/RetroshelfEngine-Bridging-Header.h` is the planned
Xcode bridging-header entry point. Once an actual Xcode app target is added,
it must configure this header, add `engine/include` to header search paths,
and link the compiled `retroshelf_engine` library. This repository change does
not create that app target or establish that a Swift-to-C++ call has run inside
an iOS app. The C ABI smoke test verifies the exported functions from C++ CI.

## Important limitations

This is **not yet a complete PlayStation 2 emulator** and cannot boot or play games. The PS2 Emotion Engine and surrounding hardware require substantial additional work, including accurate CPU behavior, exceptions and memory/bus handling, COP0/COP1/COP2, vector units, timers, DMA arbitration, full Graphics Synthesizer behavior, texture sampling/blending, sound hardware, interrupts, and BIOS integration. This initial subset is for testing the build workflow and code structure only.

`SystemBus` currently bounds reads and writes to the supplied EE-RAM vector; it is not a
PS2 memory map or an address-aliasing implementation. GIF DMA reads both chain tags and
payloads through that bus interface and submits payloads to `GsPacketProcessor`, which
parses GIF-tag framing and preserves complete packet bytes in a drainable queue bounded
to 64 MiB. It feeds the current partial GS pipeline; this is not complete GS hardware emulation. The software renderer supports only a limited set of primitive paths and is not yet driven by a complete running game. Incomplete streams are reported by `finish_stream()`.

DMA completion, tag, and error events are available through its interrupt callback. The current C ABI also exposes a synthetic graphics preview rendered through the partial GIF-to-GS pipeline; it is explicitly a test pattern, not a game frame. There
is no EE interrupt controller or architectural status/mask/acknowledgment register model
yet. GIF path arbitration, full GS behavior, and scratchpad transfers are unsupported.
CHCR.DIR must be clear for the supported EE-RAM-to-GIF direction; GIF-to-RAM transfers
are unsupported.
It models main-RAM chain tags and the two-entry CALL/RET stack, but not scratchpad
transfers, tag transfer-enable (TTE), REFS cache-snooping behavior, cycle timing,
or hardware arbitration. Payload transfers and aggregate chain processing are bounded to
64 MiB per start operation, and chain processing is bounded to 65,536 tags. No retail game compatibility has been validated. The native UI's BIOS-reset-vector and disc-execution actions are bounded smoke tests using the incomplete instruction subset, not a full BIOS boot or playable game session.

Use only BIOS and game images you are legally entitled to use. The portable engine can be developed on Linux, but a native iPhone app requires an appropriate Apple build/signing workflow, and iOS imposes restrictions on JIT execution.

## CPU branch / delay-slot subset

The test CPU now supports J, JAL, JR, JALR, BEQ, BNE, BLEZ, BGTZ, BLTZ, BGEZ, BLTZAL and BGEZAL with one-instruction
delay slots (no "likely" branches, no branch-in-delay-slot, no fetch-time address errors for unaligned jump targets).
A minimal COP0 subset (Status.EXL, Cause.ExcCode/BD, EPC) records address-error and reserved-instruction exceptions;
exceptions in a delay slot set Cause.BD and EPC to the branch address. The vector is fixed at 0x80000180. This is not a
full Emotion Engine and cannot boot a BIOS.
