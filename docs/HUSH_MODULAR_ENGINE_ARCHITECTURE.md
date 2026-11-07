# Hush PS2 Engine — Modular Architecture

## Purpose

Hush is an original PS2 emulator implementation. This document defines the subsystem boundaries and the order in which the engine will be made executable and testable. It is an engineering plan, not a claim that the emulator currently boots or plays commercial games.

## Core design rules

- Keep the implementation original; do not embed or wrap another PS2 emulator core.
- Keep hardware behavior in the C++ engine. The iOS app is a frontend for file selection, controls, display, audio, and saves.
- Give each hardware subsystem a narrow interface and deterministic unit tests.
- Prefer observable hardware state and traceable events over hidden cross-module state.
- Every milestone must have a test that demonstrates the behavior claimed.
- Do not call a BIOS inspection, ELF validation, simulator build, synthetic frame, or instruction-budget stop a successful game boot.

## Subsystem boundaries

| Subsystem | Responsibility | Current repository starting point | Required proof |
|---|---|---|---|
| EE CPU | R5900 integer pipeline, delay slots, exceptions, COP0, MMI, HI/LO | `cpu.cpp`, `cpu.hpp` | Instruction-by-instruction architectural tests and BIOS trace progress |
| EE memory / bus | Physical address map, RAM, scratchpad, ROM windows, MMIO routing | `ps2_ee_memory_map.*`, `system_bus.*` | Read/write, mirroring, alignment, and device-routing tests |
| Interrupts / timers | INTC, DMAC interrupt propagation, timers and priority | To be isolated | Register semantics and interrupt delivery tests |
| DMA | Channel registers, transfer modes, chain tags, arbitration, GIF path | `ee_gif_dma.*` | Realistic transfer descriptors and end-to-end channel tests |
| VU0 / VU1 | Vector registers, integer registers, flags, macro/micro execution | Partial VU0 macro state in CPU | Vector arithmetic tests, then microprogram control-flow tests |
| GIF | Packet parsing, packed/reglist/image modes, path arbitration | `gs_gif_pipeline.*`, `gs_packet_processor.*` | Known packet streams produce expected GS register writes |
| GS | Registers, local memory, transfers, rasterization, framebuffer | `gs_*.cpp` | Pixel-exact reference tests for supported formats and primitives |
| Disc / boot | ISO9660, SYSTEM.CNF, ELF load, EE startup state | `iso9660_disc.*`, `ps2_disc_boot_loader.*`, `ps2_disc_execution.*` | Known homebrew ELF reaches a controlled entry point and executes |
| BIOS / system boot | Reset, exception vectors, kernel services, device initialization | `ee_execution_runner.*`, BIOS validator | BIOS trace reaches known milestones without unsupported-opcode traps |
| Audio | SPU2 registers, voices, mixing, output buffering | To be implemented | Deterministic audio register and sample tests |
| Frontend | iOS Files import, touch/controller input, display, audio, save states | `ios/OriginalPS2/*` | Device test using actual engine framebuffer and input events |

## Runtime composition

The target runtime is a single deterministic machine state owned by an EE/IOP system coordinator:

1. CPU fetches and executes an instruction through the EE memory map.
2. Memory-mapped reads/writes route to RAM or a device register block.
3. DMA channels move data between memory and devices and raise interrupts.
4. GIF consumes DMA/path data and forwards decoded commands to the GS.
5. GS updates local memory and produces the actual framebuffer.
6. Timers and interrupt controllers update pending interrupt state.
7. The frontend presents the framebuffer and sends input/audio events; it does not emulate hardware.

Subsystems should not call into the iOS UI. The frontend should not directly mutate internal CPU or GS state.

## Development sequence

### Gate 0 — Reproducible baseline
- Build all host test targets and the iOS arm64 simulator library.
- Record exact compiler/test results per commit.
- Keep the current CPU instruction work isolated until its tests and encoding are independently checked.

### Gate 1 — EE execution foundation
- Define a complete CPU step result: retired instruction, exception, memory fault, or unsupported instruction.
- Add trace records with PC, opcode, delay-slot state, and exception details.
- Expand R5900 instructions using authoritative encoding tables and focused tests.
- Make memory exceptions and branch-delay behavior consistent across RAM and mapped-memory execution.

### Gate 2 — Device and interrupt framework
- Introduce explicit MMIO device interfaces and a central bus map.
- Implement INTC, timers, and DMAC register behavior.
- Test interrupt assertion, masking, priority, acknowledgement, and delivery.

### Gate 3 — Bootable test software
- Load a legally obtained homebrew test ELF into mapped EE RAM.
- Execute it through the same machine coordinator used by BIOS/game execution.
- Add small tests for memory, syscalls/exceptions, DMA, GIF, and GS output.
- Do not bypass the CPU by jumping directly to the ELF entry point for an end-to-end boot claim.

### Gate 4 — Vector and graphics execution
- Complete VU0 macro operations and build a separately tested VU micro-interpreter.
- Implement GIF path behavior and connect DMA output to the GIF parser.
- Connect GS register writes, transfers, local memory, and rasterizer to the real framebuffer.
- Verify pixels against deterministic test scenes.

### Gate 5 — BIOS and game compatibility
- Run the user's own BIOS through reset and exception handling.
- Implement required kernel/device behavior based on traces.
- Run a legally obtained homebrew title first, then broaden compatibility.
- Record boot logs and unsupported operations; never mark a title playable until it reaches interactive gameplay and renders real frames.

### Gate 6 — iPhone integration
- Expose a stable C ABI for machine lifecycle, frame stepping, framebuffer, input, audio, and save states.
- Compile for iOS arm64 device and simulator.
- Validate thermal/performance behavior on the actual iPhone.
- Package the app only after the engine API and frontend are connected.

## CI requirements

Keep separate checks for:
- host C++ unit tests;
- sanitizer/debug tests where supported;
- iOS arm64 simulator cross-compilation;
- iOS device compilation and unsigned IPA packaging.

A green compile job is only a compile result. A green unit-test job proves only the tested components. The release checklist must separately report BIOS boot, homebrew execution, real GS frames, audio, controls, and interactive gameplay.

## Current known limitations

The current codebase contains useful independent foundations for CPU execution, EE memory, ISO/ELF loading, DMA/GIF/GS processing, and software rasterization. These components are not yet integrated into a complete PS2 machine. The current CPU is partial, VU micro execution and broad device behavior remain incomplete, and the current BIOS/game runners are bounded diagnostic runners rather than proof of a successful boot. No playable-game claim is made by this architecture document.
