# PS2 engine status and next milestones

This document distinguishes implemented prototype paths from features required for a playable PlayStation 2 emulator.

## Verified prototype paths

- The C++ engine configures and builds in GitHub Actions; automated tests and the demonstration step passed in workflow run [38609042966](https://github.com/laurenlindsay588-hash/PS2-and-PS3-iPhone-Emulator/actions/runs/38609042966).
- The EE CPU executes a limited MIPS-like instruction subset with a deliberately simplified one-instruction/one-cycle timing model.
- The system bus maps prototype RAM and an explicitly supplied BIOS ROM window; tests cover ROM reads and write protection.
- The ISO9660 reader can locate a PS2 boot executable through SYSTEM.CNF, and the ELF loader can load supported segments into prototype RAM. This does **not** implement the BIOS boot sequence or PS2 runtime services.
- MIPS KSEG0/KSEG1 aliases are translated for the supported RAM access path.
- The GIF parser and processor accept a limited PACKED-mode stream containing PRIM, RGBAQ, and XYZ2 writes for untextured triangle-list primitives. A synthetic test confirms that a GIF packet read from EE RAM can draw one triangle into the software framebuffer.
- The iOS shell has a synthetic GIF preview path. A successful engine CI run is not proof that the iOS app builds, is signed, installs, or runs on a physical iPhone.

## Explicit limitations

- `EeMachine::process_gif_from_ram` is a RAM-to-parser bridge, **not** the PS2 DMAC GIF channel. It does not implement CHCR/MADR/QWC/TADR registers, DMA channel arbitration, tag-chain modes, stalls, or DMA interrupts.
- The GS is not hardware-accurate. The current processor does not implement the complete GS register set, all GIF formats, textures, blending, depth testing, CLUTs, or the full rasterization rules.
- The EE CPU, timers, interrupts, and bus are partial models. VU0/VU1, IOP, SIF, IPU, SPU2, and the complete PS2 memory map are not implemented.
- Loading a BIOS file or finding an ELF in an ISO is not the same as booting a retail game. There is no verified retail-game boot or gameplay result.
- The iOS preview is synthetic; it is not a live framebuffer from a running game. Touch/controller input, save states, memory cards, and per-game settings are not connected to a complete emulation session.

## Next engineering milestones

1. Implement and test the PS2 EE DMAC register map and channel-2 GIF normal-mode transfers, including address/size validation, completion state, and interrupt signaling.
2. Add GIF chain mode and tag parsing, with explicit tests for CNT, NEXT, REF, REFS, CALL, RET, END, and malformed chains before forwarding payloads to the GIF processor.
3. Expand GS state and primitive coverage behind tests; reject unsupported commands explicitly rather than silently rendering incorrect output.
4. Improve EE instruction coverage and exception behavior, then implement the IOP/SIF and VU execution paths needed by the BIOS and games.
5. Connect the native iOS view to a live engine framebuffer and input state, then validate the app workflow and device installation.
6. Test with a BIOS and game image the user is legally entitled to use; record boot and gameplay results separately from synthetic unit tests.

## Safety and compatibility notes

Use BIOS and game images you are legally entitled to use. This prototype is not currently a replacement for a mature PS2 emulator and should not be advertised as playable until real-game boot and sustained gameplay are demonstrated.
