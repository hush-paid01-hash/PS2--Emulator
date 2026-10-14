# RetroShelf Engine Foundation

This folder is the first portable C++ milestone for the original PS2-emulator project.

## Current scope

- A tiny instruction interpreter for a limited MIPS-style subset.
- General-purpose register state and a simple program counter.
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

## Important limitations

This is **not yet a complete PlayStation 2 emulator** and cannot boot or play games. The PS2 Emotion Engine requires substantial additional work, including accurate CPU behavior, exceptions, memory/bus handling, COP0/COP1/COP2, vector units, timers, DMA, the Graphics Synthesizer, sound hardware, and BIOS integration. This initial subset is for testing the build workflow and code structure only.

Use only BIOS and game images you are legally entitled to use. The portable engine can be developed on Linux, but a native iPhone app requires an appropriate Apple build/signing workflow, and iOS imposes restrictions on JIT execution.
