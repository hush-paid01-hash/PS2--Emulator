# RetroShelf Engine Foundation

This folder is the first portable C++ milestone for the original PS2-emulator project.

## Current scope

- A tiny instruction interpreter for a limited MIPS-style subset.
- General-purpose register state and a simple program counter.
- An EE GIF DMA channel model with normal transfers and DMA chain tags.
- A bounds-checked system-bus interface over the current EE RAM allocation.
- A stateful GIF-tag packet parser for PACKED, REGLIST, IMAGE, and IMAGE2 streams.
- A command-line demonstration and automated tests.
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

This is **not yet a complete PlayStation 2 emulator** and cannot boot or play games. The PS2 Emotion Engine requires substantial additional work, including accurate CPU behavior, exceptions, memory/bus handling, COP0/COP1/COP2, vector units, timers, DMA, the Graphics Synthesizer, sound hardware, and BIOS integration. This initial subset is for testing the build workflow and code structure only.

`SystemBus` currently bounds reads and writes to the supplied EE-RAM vector; it is not a
PS2 memory map or an address-aliasing implementation. GIF DMA reads both chain tags and
payloads through that bus interface and submits payloads to `GsPacketProcessor`, which
parses GIF-tag framing and preserves complete packet bytes in a drainable queue bounded
to 64 MiB. It is a packet parser, not a GS register implementation or renderer. Incomplete
streams are reported by `finish_stream()`.

DMA completion, tag, and error events are available through its interrupt callback. There
is no EE interrupt controller or architectural status/mask/acknowledgment register model
yet. GIF path arbitration, full GS behavior, and scratchpad transfers are unsupported.
CHCR.DIR must be clear for the supported EE-RAM-to-GIF direction; GIF-to-RAM transfers
are unsupported.
It models main-RAM chain tags and the two-entry CALL/RET stack, but not scratchpad
transfers, tag transfer-enable (TTE), REFS cache-snooping behavior, cycle timing,
or hardware arbitration. Payload transfers and aggregate chain processing are bounded to
64 MiB per start operation, and chain processing is bounded to 65,536 tags. No retail game
compatibility has been validated.

Use only BIOS and game images you are legally entitled to use. The portable engine can be developed on Linux, but a native iPhone app requires an appropriate Apple build/signing workflow, and iOS imposes restrictions on JIT execution.

## CPU branch / delay-slot subset

The test CPU now supports J, JAL, JR, JALR, BEQ, BNE, BLEZ, BGTZ, BLTZ, BGEZ, BLTZAL and BGEZAL with one-instruction
delay slots (no "likely" branches, no branch-in-delay-slot, no fetch-time address errors for unaligned jump targets).
A minimal COP0 subset (Status.EXL, Cause.ExcCode/BD, EPC) records address-error and reserved-instruction exceptions;
exceptions in a delay slot set Cause.BD and EPC to the branch address. The vector is fixed at 0x80000180. This is not a
full Emotion Engine and cannot boot a BIOS.
