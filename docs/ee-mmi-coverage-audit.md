# Emotion Engine MMI coverage audit

Status: engineering audit; not a claim of complete R5900/MMI emulation.

This document records the implementation visible in `engine/src/cpu.cpp` and the gaps that must be closed before BIOS/game compatibility can be claimed.

## Existing partial implementation

The current `Cpu::step` decoder has code paths for:

- MMI0 (`SPECIAL2`, function `0x08`): packed add/subtract and signed greater-than/max lane operations for selected 32-, 16-, and 8-bit sub-op encodings.
- MMI1 (function `0x28`): packed equality comparisons for selected 32-, 16-, and 8-bit sub-op encodings.
- MMI2/MMI3 (functions `0x09`/`0x29`): selected 128-bit bitwise operations, variable word shifts, and fixed packed halfword/word shifts.
- 128-bit GPR values are represented as independent low/high 64-bit halves.

These are selected subsets, not complete MMI0/MMI1/MMI2/MMI3 coverage. Unsupported encodings currently raise a reserved-instruction exception.

## Required next work

1. Build an instruction/encoding table from authoritative R5900 documentation for every MMI0/MMI1/MMI2/MMI3 operation, including function, sub-op, operand fields, lane width, signedness, saturation and accumulator effects.
2. Add independent reference-vector tests for every implemented encoding. Include low and high 64-bit halves, GPR zero, boundary values, saturation edges, and shift counts at 0 and lane-width minus one.
3. Implement missing operations only after their exact encoding and arithmetic semantics are confirmed. Do not infer an instruction from a mnemonic or neighboring sub-op.
4. Verify 128-bit accumulator and HI/LO interactions separately; do not treat all packed operations as simple lane arithmetic.
5. Run the complete CMake/CTest suite on the target branch and record the exact results.

## Important compatibility gaps

The CPU header labels the current implementation as an educational milestone. The interpreter also models only a subset of COP0 and VU0 behavior, and its comments explicitly state that VU micro execution is not implemented. Completing MMI alone will not make the Emotion Engine complete or establish BIOS/game compatibility.

## Acceptance criteria

- Every supported encoding has at least one positive test and relevant boundary tests.
- Reserved/unsupported encodings are rejected deterministically.
- The complete existing test suite passes.
- No claim of playable emulation is made until a legally obtained BIOS boots and real PS2 software executes end-to-end with meaningful graphics/audio output.
