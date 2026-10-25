#include "retroshelf/cpu.hpp"
#include "retroshelf/elf_ram_loader.hpp"
#include "retroshelf/ps2_ee_memory_map.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
void append_instruction(std::vector<std::uint8_t>& bytes, std::uint32_t instruction) {
    bytes.push_back(static_cast<std::uint8_t>(instruction));
    bytes.push_back(static_cast<std::uint8_t>(instruction >> 8U));
    bytes.push_back(static_cast<std::uint8_t>(instruction >> 16U));
    bytes.push_back(static_cast<std::uint8_t>(instruction >> 24U));
}
}

int main() {
    retroshelf::Elf32MipsImage image;
    image.entry_point = 0x1000;
    retroshelf::Elf32LoadSegment segment;
    segment.virtual_address = 0x1000;
    segment.memory_size = 12;
    segment.flags = 5; // readable + executable
    append_instruction(segment.file_bytes, (0x09U << 26U) | (8U << 16U) | 7U);
    append_instruction(segment.file_bytes, (0x09U << 26U) | (9U << 16U) | 5U);
    image.load_segments.push_back(segment);

    std::vector<std::uint8_t> ram(0x2000, 0xA5);
    std::uint32_t entry = 0;
    std::string error;
    assert(retroshelf::load_elf32_into_ram(image, 0, ram, entry, &error));
    assert(entry == 0x1000);
    assert(ram[0x1000] == 0x07);
    assert(ram[0x1004] == 0x05);
    assert(ram[0x1008] == 0);

    retroshelf::Cpu cpu;
    cpu.set_pc(entry);
    assert(cpu.step_from_ram(ram, 0, &error));
    assert(cpu.reg(8) == 7);
    assert(cpu.pc() == 0x1004);
    assert(cpu.step_from_ram(ram, 0, &error));
    assert(cpu.reg(9) == 5);
    assert(cpu.pc() == 0x1008);
    // Execute from KSEG0 and prove LW/SW share memory through the KSEG1 alias.
    retroshelf::Ps2EeMemoryMap ee;
    retroshelf::Cpu mapped_cpu;
    mapped_cpu.set_pc(0x80001000U);
    mapped_cpu.set_reg(16, 0xA0000000U); // s0 points at KSEG1 RAM alias
    assert(ee.write(0x80001000U, 4, (0x09U << 26U) | (8U << 16U) | 7U, &error));
    assert(ee.write(0x80001004U, 4, (0x2BU << 26U) | (16U << 21U) | (8U << 16U) | 0x20U, &error)); // SW t0,0x20(s0)
    assert(ee.write(0x80001008U, 4, (0x23U << 26U) | (16U << 21U) | (9U << 16U) | 0x20U, &error)); // LW t1,0x20(s0)
    assert(mapped_cpu.step_from_ee_memory(ee, &error));
    assert(mapped_cpu.reg(8) == 7);
    assert(mapped_cpu.step_from_ee_memory(ee, &error));
    std::uint64_t stored = 0;
    assert(ee.read(0x00000020U, 4, stored, &error));
    assert(stored == 7);
    assert(mapped_cpu.step_from_ee_memory(ee, &error));
    assert(mapped_cpu.reg(9) == 7);

    // Reject addresses that would otherwise be truncated to a valid 32-bit
    // EE address by the current memory-map interface.
    retroshelf::Cpu high_pc_cpu;
    high_pc_cpu.set_pc(0x180001000ULL);
    assert(!high_pc_cpu.step_from_ee_memory(ee, &error));
    assert(error.find("32-bit EE address space") != std::string::npos);
    assert(high_pc_cpu.cop0_bad_vaddr() == 0x180001000ULL);

    retroshelf::Cpu high_data_cpu;
    high_data_cpu.set_pc(0x80001000U);
    high_data_cpu.set_reg(16, 0x100000000ULL);
    assert(ee.write(0x80001000U, 4,
                    (0x2BU << 26U) | (16U << 21U) | (8U << 16U), &error)); // SW t0,0(s0)
    assert(!high_data_cpu.step_from_ee_memory(ee, &error));
    assert(error.find("32-bit address space") != std::string::npos);
    assert(ee.read(0x00000000U, 4, stored, &error));
    assert(stored == 0U); // rejected store must not corrupt physical address zero

    // Unmapped EE data loads/stores must raise the expected address exception.
    retroshelf::Cpu bad_load_cpu;
    bad_load_cpu.set_pc(0x80002000U);
    bad_load_cpu.set_reg(16, 0x30000000U); // outside the currently mapped RAM aliases
    assert(ee.write(0x80002000U, 4,
                    (0x23U << 26U) | (16U << 21U) | (9U << 16U), &error)); // LW t1,0(s0)
    assert(!bad_load_cpu.step_from_ee_memory(ee, &error));
    assert(bad_load_cpu.cop0_bad_vaddr() == 0x30000000U);
    assert((bad_load_cpu.cop0_cause() & (0x1FU << 2U)) ==
           (retroshelf::Cpu::ExcAddressErrorLoad << 2U));
    assert((bad_load_cpu.cop0_status() & retroshelf::Cpu::StatusExl) != 0U);
    assert(bad_load_cpu.cop0_epc() == 0x80002000U);
    assert(bad_load_cpu.pc() == retroshelf::Cpu::GeneralExceptionVector);

    retroshelf::Cpu bad_store_cpu;
    bad_store_cpu.set_pc(0x80003000U);
    bad_store_cpu.set_reg(16, 0x30000000U);
    assert(ee.write(0x80003000U, 4,
                    (0x2BU << 26U) | (16U << 21U) | (8U << 16U), &error)); // SW t0,0(s0)
    assert(!bad_store_cpu.step_from_ee_memory(ee, &error));
    assert((bad_store_cpu.cop0_cause() & (0x1FU << 2U)) ==
           (retroshelf::Cpu::ExcAddressErrorStore << 2U));
    assert((bad_store_cpu.cop0_status() & retroshelf::Cpu::StatusExl) != 0U);
    assert(bad_store_cpu.cop0_epc() == 0x80003000U);
    assert(bad_store_cpu.pc() == retroshelf::Cpu::GeneralExceptionVector);

    // A memory fault in a branch delay slot records the branch PC in EPC
    // and sets Cause.BD, rather than reporting the delay-slot PC as the origin.
    retroshelf::Cpu delay_fault_cpu;
    delay_fault_cpu.set_pc(0x80004000U);
    delay_fault_cpu.set_reg(16, 0x30000000U);
    assert(ee.write(0x80004000U, 4, (0x04U << 26U), &error)); // BEQ zero,zero,0
    assert(ee.write(0x80004004U, 4,
                    (0x23U << 26U) | (16U << 21U) | (9U << 16U), &error)); // LW t1,0(s0)
    assert(delay_fault_cpu.step_from_ee_memory(ee, &error));
    assert(delay_fault_cpu.in_delay_slot());
    assert(!delay_fault_cpu.step_from_ee_memory(ee, &error));
    assert((delay_fault_cpu.cop0_cause() & retroshelf::Cpu::CauseBd) != 0U);
    assert((delay_fault_cpu.cop0_cause() & (0x1FU << 2U)) ==
           (retroshelf::Cpu::ExcAddressErrorLoad << 2U));
    assert(delay_fault_cpu.cop0_epc() == 0x80004000U);
    assert(delay_fault_cpu.pc() == retroshelf::Cpu::GeneralExceptionVector);

    // Unsupported instructions should raise Reserved Instruction and save EPC.
    retroshelf::Cpu reserved_cpu;
    reserved_cpu.set_pc(0x80005000U);
    assert(ee.write(0x80005000U, 4, 0x3EU << 26U, &error));
    assert(!reserved_cpu.step_from_ee_memory(ee, &error));
    assert((reserved_cpu.cop0_cause() & (0x1FU << 2U)) ==
           (retroshelf::Cpu::ExcReservedInstruction << 2U));
    assert(reserved_cpu.cop0_epc() == 0x80005000U);
    assert((reserved_cpu.cop0_status() & retroshelf::Cpu::StatusExl) != 0U);
    assert(reserved_cpu.pc() == retroshelf::Cpu::GeneralExceptionVector);

    // Misaligned word accesses must raise Address Error rather than read/write
    // partially overlapping RAM bytes.
    retroshelf::Cpu unaligned_load_cpu;
    unaligned_load_cpu.set_pc(0x80006000U);
    unaligned_load_cpu.set_reg(16, 0x21U);
    assert(ee.write(0x80006000U, 4,
                    (0x23U << 26U) | (16U << 21U) | (9U << 16U), &error)); // LW t1,0(s0)
    assert(!unaligned_load_cpu.step_from_ee_memory(ee, &error));
    assert((unaligned_load_cpu.cop0_cause() & (0x1FU << 2U)) ==
           (retroshelf::Cpu::ExcAddressErrorLoad << 2U));
    assert(unaligned_load_cpu.cop0_epc() == 0x80006000U);
    assert((unaligned_load_cpu.cop0_status() & retroshelf::Cpu::StatusExl) != 0U);

    retroshelf::Cpu unaligned_store_cpu;
    unaligned_store_cpu.set_pc(0x80007000U);
    unaligned_store_cpu.set_reg(16, 0x21U);
    assert(ee.write(0x80007000U, 4,
                    (0x2BU << 26U) | (16U << 21U) | (8U << 16U), &error)); // SW t0,0(s0)
    assert(!unaligned_store_cpu.step_from_ee_memory(ee, &error));
    assert((unaligned_store_cpu.cop0_cause() & (0x1FU << 2U)) ==
           (retroshelf::Cpu::ExcAddressErrorStore << 2U));
    assert(unaligned_store_cpu.cop0_epc() == 0x80007000U);
    assert((unaligned_store_cpu.cop0_status() & retroshelf::Cpu::StatusExl) != 0U);


    // Halfword accesses also require two-byte alignment and must report the
    // load/store-specific Address Error without modifying neighboring bytes.
    assert(ee.write(0x00000020U, 4, 0xA1B2C3D4U, &error));
    retroshelf::Cpu unaligned_half_load_cpu;
    unaligned_half_load_cpu.set_pc(0x80008000U);
    unaligned_half_load_cpu.set_reg(16, 0x21U);
    assert(ee.write(0x80008000U, 4,
                    (0x21U << 26U) | (16U << 21U) | (9U << 16U), &error)); // LH t1,0(s0)
    assert(!unaligned_half_load_cpu.step_from_ee_memory(ee, &error));
    assert((unaligned_half_load_cpu.cop0_cause() & (0x1FU << 2U)) ==
           (retroshelf::Cpu::ExcAddressErrorLoad << 2U));
    assert(unaligned_half_load_cpu.cop0_epc() == 0x80008000U);
    assert((unaligned_half_load_cpu.cop0_status() & retroshelf::Cpu::StatusExl) != 0U);

    retroshelf::Cpu unaligned_half_store_cpu;
    unaligned_half_store_cpu.set_pc(0x80009000U);
    unaligned_half_store_cpu.set_reg(16, 0x21U);
    unaligned_half_store_cpu.set_reg(8, 0xFFFFFFFFU);
    assert(ee.write(0x80009000U, 4,
                    (0x29U << 26U) | (16U << 21U) | (8U << 16U), &error)); // SH t0,0(s0)
    assert(!unaligned_half_store_cpu.step_from_ee_memory(ee, &error));
    assert((unaligned_half_store_cpu.cop0_cause() & (0x1FU << 2U)) ==
           (retroshelf::Cpu::ExcAddressErrorStore << 2U));
    assert(unaligned_half_store_cpu.cop0_epc() == 0x80009000U);
    assert((unaligned_half_store_cpu.cop0_status() & retroshelf::Cpu::StatusExl) != 0U);
    assert(ee.read(0x00000020U, 4, stored, &error));
    assert(stored == 0xA1B2C3D4U); // failed SH leaves the original word untouched


    // Byte accesses may use any address; SB changes one byte, LB sign-extends,
    // and LBU zero-extends the same high-bit-set byte.
    retroshelf::Cpu byte_cpu;
    byte_cpu.set_pc(0x8000A000U);
    byte_cpu.set_reg(16, 0x25U);
    byte_cpu.set_reg(8, 0x80U);
    assert(ee.write(0x8000A000U, 4,
                    (0x28U << 26U) | (16U << 21U) | (8U << 16U), &error)); // SB t0,0(s0)
    assert(ee.write(0x8000A004U, 4,
                    (0x20U << 26U) | (16U << 21U) | (9U << 16U), &error)); // LB t1,0(s0)
    assert(ee.write(0x8000A008U, 4,
                    (0x24U << 26U) | (16U << 21U) | (10U << 16U), &error)); // LBU t2,0(s0)
    assert(byte_cpu.step_from_ee_memory(ee, &error));
    assert(ee.read(0x25U, 1, stored, &error));
    assert(stored == 0x80U);
    assert(byte_cpu.step_from_ee_memory(ee, &error));
    assert(byte_cpu.reg(9) == 0xFFFFFFFFFFFFFF80ULL);
    assert(byte_cpu.step_from_ee_memory(ee, &error));
    assert(byte_cpu.reg(10) == 0x80U);

    // MULT/MULTU write the 64-bit product to HI:LO; MFHI/MFLO expose halves.
    retroshelf::Cpu multu_cpu;
    multu_cpu.set_pc(0x8000B000U);
    multu_cpu.set_reg(8, 0xFFFFFFFFU);
    multu_cpu.set_reg(9, 2U);
    assert(ee.write(0x8000B000U, 4, (8U << 21U) | (9U << 16U) | 0x19U, &error)); // MULTU t0,t1
    assert(ee.write(0x8000B004U, 4, (10U << 11U) | 0x12U, &error)); // MFLO t2
    assert(ee.write(0x8000B008U, 4, (11U << 11U) | 0x10U, &error)); // MFHI t3
    assert(multu_cpu.step_from_ee_memory(ee, &error));
    assert(multu_cpu.lo() == 0xFFFFFFFEU);
    assert(multu_cpu.hi() == 1U);
    assert(multu_cpu.step_from_ee_memory(ee, &error));
    assert(multu_cpu.reg(10) == 0xFFFFFFFEU);
    assert(multu_cpu.step_from_ee_memory(ee, &error));
    assert(multu_cpu.reg(11) == 1U);

    // Signed MULT of -2 by 3 yields the two's-complement 64-bit product.
    retroshelf::Cpu mult_cpu;
    mult_cpu.set_pc(0x8000B100U);
    mult_cpu.set_reg(8, 0xFFFFFFFEU);
    mult_cpu.set_reg(9, 3U);
    assert(ee.write(0x8000B100U, 4, (8U << 21U) | (9U << 16U) | 0x18U, &error)); // MULT t0,t1
    assert(mult_cpu.step_from_ee_memory(ee, &error));
    assert(mult_cpu.lo() == 0xFFFFFFFFFFFFFFFAULL);
    assert(mult_cpu.hi() == 0xFFFFFFFFFFFFFFFFULL);

    // DIV/DIVU place quotient in LO and remainder in HI.
    retroshelf::Cpu div_cpu;
    div_cpu.set_pc(0x8000B200U);
    div_cpu.set_reg(8, static_cast<std::uint32_t>(-17));
    div_cpu.set_reg(9, 5U);
    assert(ee.write(0x8000B200U, 4, (8U << 21U) | (9U << 16U) | 0x1AU, &error)); // DIV t0,t1
    assert(div_cpu.step_from_ee_memory(ee, &error));
    assert(div_cpu.lo() == static_cast<std::uint64_t>(static_cast<std::int64_t>(-3)));
    assert(div_cpu.hi() == static_cast<std::uint64_t>(static_cast<std::int64_t>(-2)));

    retroshelf::Cpu divu_cpu;
    divu_cpu.set_pc(0x8000B300U);
    divu_cpu.set_reg(8, 17U);
    divu_cpu.set_reg(9, 5U);
    assert(ee.write(0x8000B300U, 4, (8U << 21U) | (9U << 16U) | 0x1BU, &error)); // DIVU t0,t1
    assert(divu_cpu.step_from_ee_memory(ee, &error));
    assert(divu_cpu.lo() == 3U);
    assert(divu_cpu.hi() == 2U);

    // Division by zero is handled deterministically and must not crash.
    retroshelf::Cpu div_zero_cpu;
    div_zero_cpu.set_pc(0x8000B400U);
    div_zero_cpu.set_reg(8, 7U);
    div_zero_cpu.set_reg(9, 0U);
    assert(ee.write(0x8000B400U, 4, (8U << 21U) | (9U << 16U) | 0x1AU, &error));
    assert(div_zero_cpu.step_from_ee_memory(ee, &error));
    assert(div_zero_cpu.lo() == 0xFFFFFFFFFFFFFFFFULL);
    assert(div_zero_cpu.hi() == 7U);

    // MTHI/MTLO move general registers into the special HI/LO registers.
    retroshelf::Cpu move_hilo_cpu;
    move_hilo_cpu.set_pc(0x8000B500U);
    move_hilo_cpu.set_reg(8, 0x12345678ABCDEF01ULL);
    move_hilo_cpu.set_reg(9, 0xFEDCBA9876543210ULL);
    assert(ee.write(0x8000B500U, 4, (8U << 21U) | 0x11U, &error)); // MTHI t0
    assert(ee.write(0x8000B504U, 4, (9U << 21U) | 0x13U, &error)); // MTLO t1
    assert(ee.write(0x8000B508U, 4, (10U << 11U) | 0x10U, &error)); // MFHI t2
    assert(ee.write(0x8000B50CU, 4, (11U << 11U) | 0x12U, &error)); // MFLO t3
    assert(move_hilo_cpu.step_from_ee_memory(ee, &error));
    assert(move_hilo_cpu.hi() == 0x12345678ABCDEF01ULL);
    assert(move_hilo_cpu.step_from_ee_memory(ee, &error));
    assert(move_hilo_cpu.lo() == 0xFEDCBA9876543210ULL);
    assert(move_hilo_cpu.step_from_ee_memory(ee, &error));
    assert(move_hilo_cpu.reg(10) == 0x12345678ABCDEF01ULL);
    assert(move_hilo_cpu.step_from_ee_memory(ee, &error));
    assert(move_hilo_cpu.reg(11) == 0xFEDCBA9876543210ULL);

    // Minimal COP0 moves expose EPC and ERET returns to it while clearing EXL.
    retroshelf::Cpu cop0_cpu;
    cop0_cpu.set_pc(0x8000C000U);
    cop0_cpu.set_reg(8, 0x8000D000U);
    assert(!cop0_cpu.step(0xFFFFFFFFU, &error)); // reserved instruction records EPC
    assert(cop0_cpu.cop0_epc() == 0x8000C000U);
    assert(cop0_cpu.step((0x10U << 26U) | (8U << 16U) | (14U << 11U), &error)); // MFC0 t0,EPC
    assert(cop0_cpu.reg(8) == 0xFFFFFFFF8000C000ULL); // MFC0 sign-extends
    assert(cop0_cpu.step((0x10U << 26U) | (4U << 21U) | (8U << 16U) | (14U << 11U), &error)); // MTC0 t0,EPC
    assert(cop0_cpu.cop0_epc() == 0xFFFFFFFF8000C000ULL);
    assert(cop0_cpu.step((0x10U << 26U) | (16U << 21U) | 0x18U, &error)); // ERET
    assert(cop0_cpu.pc() == 0xFFFFFFFF8000C000ULL);
    assert((cop0_cpu.cop0_status() & retroshelf::Cpu::StatusExl) == 0U);

    // LWL/LWR/SWL/SWR merge each byte lane in little-endian order.
    const std::uint32_t merge_base = 0x00000100U;
    const std::uint32_t merge_pc = 0x8000C100U;
    const std::uint32_t lwl_expected[] = {0x11BBCCDDU, 0x2211CCDDU, 0x332211DDU, 0x44332211U};
    const std::uint32_t lwr_expected[] = {0x44332211U, 0xAA443322U, 0xAABB4433U, 0xAABBCC44U};
    const std::uint32_t swl_expected[] = {0x443322AAU, 0x4433AABBU, 0x44AABBCCU, 0xAABBCCDDU};
    const std::uint32_t swr_expected[] = {0xAABBCCDDU, 0xBBCCDD11U, 0xCCDD2211U, 0xDD332211U};
    const std::uint32_t merge_opcodes[] = {0x22U, 0x26U, 0x2AU, 0x2EU};
    const std::uint32_t* merge_expected[] = {lwl_expected, lwr_expected, swl_expected, swr_expected};
    for (unsigned op = 0; op < 4; ++op) {
        for (unsigned lane = 0; lane < 4; ++lane) {
            const std::uint32_t test_pc = merge_pc + op * 0x40U + lane * 4U;
            assert(ee.write(merge_base, 4, 0x44332211U, &error));
            assert(ee.write(test_pc, 4, (merge_opcodes[op] << 26U) | (16U << 21U) | (8U << 16U) | lane, &error));
            retroshelf::Cpu merge_cpu;
            merge_cpu.set_pc(test_pc);
            merge_cpu.set_reg(16, merge_base);
            merge_cpu.set_reg(8, 0xAABBCCDDU);
            assert(merge_cpu.step_from_ee_memory(ee, &error));
            std::uint64_t merged = 0;
            assert(ee.read(merge_base, 4, merged, &error));
            if (op < 2) {
                assert(static_cast<std::uint32_t>(merge_cpu.reg(8)) == merge_expected[op][lane]);
                assert(merged == 0x44332211U); // loads never modify memory
            } else {
                assert(merged == merge_expected[op][lane]);
            }
        }
    }


    // LDL/LDR/SDL/SDR merge all eight byte lanes of a little-endian doubleword.
    const std::uint64_t dword_merge_memory = 0x8877665544332211ULL;
    const std::uint64_t dword_merge_register = 0xAABBCCDDEEFF0011ULL;
    const std::uint64_t ldl_expected[] = {
        0x11BBCCDDEEFF0011ULL, 0x2211CCDDEEFF0011ULL,
        0x332211DDEEFF0011ULL, 0x44332211EEFF0011ULL,
        0x5544332211FF0011ULL, 0x6655443322110011ULL,
        0x7766554433221111ULL, 0x8877665544332211ULL};
    const std::uint64_t ldr_expected[] = {
        0x8877665544332211ULL, 0xAA88776655443322ULL,
        0xAABB887766554433ULL, 0xAABBCC8877665544ULL,
        0xAABBCCDD88776655ULL, 0xAABBCCDDEE887766ULL,
        0xAABBCCDDEEFF8877ULL, 0xAABBCCDDEEFF0088ULL};
    const std::uint64_t sdl_expected[] = {
        0x88776655443322AAULL, 0x887766554433AABBULL,
        0x8877665544AABBCCULL, 0x88776655AABBCCDDULL,
        0x887766AABBCCDDEEULL, 0x8877AABBCCDDEEFFULL,
        0x88AABBCCDDEEFF00ULL, 0xAABBCCDDEEFF0011ULL};
    const std::uint64_t sdr_expected[] = {
        0xAABBCCDDEEFF0011ULL, 0xBBCCDDEEFF001111ULL,
        0xCCDDEEFF00112211ULL, 0xDDEEFF0011332211ULL,
        0xEEFF001144332211ULL, 0xFF00115544332211ULL,
        0x0011665544332211ULL, 0x1177665544332211ULL};
    const std::uint32_t dword_merge_opcodes[] = {0x1AU, 0x1BU, 0x2CU, 0x2DU};
    const std::uint64_t* dword_merge_expected[] = {
        ldl_expected, ldr_expected, sdl_expected, sdr_expected};
    for (unsigned op = 0; op < 4; ++op) {
        for (unsigned lane = 0; lane < 8; ++lane) {
            const std::uint32_t test_pc = 0x8000D000U + op * 0x40U + lane * 4U;
            assert(ee.write(merge_base, 8, dword_merge_memory, &error));
            assert(ee.write(test_pc, 4,
                            (dword_merge_opcodes[op] << 26U) |
                            (16U << 21U) | (8U << 16U) | lane, &error));
            retroshelf::Cpu merge_cpu;
            merge_cpu.set_pc(test_pc);
            merge_cpu.set_reg(16, merge_base);
            merge_cpu.set_reg(8, dword_merge_register);
            assert(merge_cpu.step_from_ee_memory(ee, &error));
            std::uint64_t merged = 0;
            assert(ee.read(merge_base, 8, merged, &error));
            if (op < 2) {
                assert(merge_cpu.reg(8) == dword_merge_expected[op][lane]);
                assert(merged == dword_merge_memory);
            } else {
                assert(merged == dword_merge_expected[op][lane]);
            }
        }
    }


    // Signed ADD, SUB, and ADDI trap on 32-bit overflow; unsigned variants do not.
    retroshelf::Cpu add_overflow_cpu;
    add_overflow_cpu.set_pc(0x8000E000U);
    add_overflow_cpu.set_reg(8, 0x7FFFFFFFU);
    add_overflow_cpu.set_reg(9, 1U);
    assert(!add_overflow_cpu.step((8U << 21U) | (9U << 16U) | (10U << 11U) | 0x20U, &error)); // ADD t2,t0,t1
    assert((add_overflow_cpu.cop0_cause() & (0x1FU << 2U)) ==
           (retroshelf::Cpu::ExcOverflow << 2U));
    assert(add_overflow_cpu.cop0_epc() == 0x8000E000U);
    assert((add_overflow_cpu.cop0_status() & retroshelf::Cpu::StatusExl) != 0U);
    assert(add_overflow_cpu.reg(10) == 0U); // destination is unchanged on overflow

    retroshelf::Cpu sub_overflow_cpu;
    sub_overflow_cpu.set_pc(0x8000E100U);
    sub_overflow_cpu.set_reg(8, 0x80000000U);
    sub_overflow_cpu.set_reg(9, 1U);
    assert(!sub_overflow_cpu.step((8U << 21U) | (9U << 16U) | (10U << 11U) | 0x22U, &error)); // SUB t2,t0,t1
    assert((sub_overflow_cpu.cop0_cause() & (0x1FU << 2U)) ==
           (retroshelf::Cpu::ExcOverflow << 2U));
    assert(sub_overflow_cpu.cop0_epc() == 0x8000E100U);
    assert(sub_overflow_cpu.reg(10) == 0U);

    retroshelf::Cpu addi_overflow_cpu;
    addi_overflow_cpu.set_pc(0x8000E200U);
    addi_overflow_cpu.set_reg(8, 0x7FFFFFFFU);
    assert(!addi_overflow_cpu.step((0x08U << 26U) | (8U << 21U) | (9U << 16U) | 1U, &error)); // ADDI t1,t0,1
    assert((addi_overflow_cpu.cop0_cause() & (0x1FU << 2U)) ==
           (retroshelf::Cpu::ExcOverflow << 2U));
    assert(addi_overflow_cpu.cop0_epc() == 0x8000E200U);
    assert(addi_overflow_cpu.reg(9) == 0U);

    retroshelf::Cpu nonoverflow_cpu;
    nonoverflow_cpu.set_pc(0x8000E300U);
    nonoverflow_cpu.set_reg(8, 0x7FFFFFFFU);
    nonoverflow_cpu.set_reg(9, 1U);
    assert(nonoverflow_cpu.step((8U << 21U) | (9U << 16U) | (10U << 11U) | 0x21U, &error)); // ADDU wraps
    assert(nonoverflow_cpu.reg(10) == 0xFFFFFFFF80000000ULL);
    assert(nonoverflow_cpu.step((0x09U << 26U) | (8U << 21U) | (11U << 16U) | 1U, &error)); // ADDIU wraps
    assert(nonoverflow_cpu.reg(11) == 0xFFFFFFFF80000000ULL);


    // SYSCALL and BREAK raise their own exception codes and save the fault PC.
    retroshelf::Cpu syscall_cpu;
    syscall_cpu.set_pc(0x8000F000U);
    assert(!syscall_cpu.step(0x0000000CU, &error)); // SYSCALL
    assert((syscall_cpu.cop0_cause() & (0x1FU << 2U)) ==
           (retroshelf::Cpu::ExcSyscall << 2U));
    assert(syscall_cpu.cop0_epc() == 0x8000F000U);
    assert((syscall_cpu.cop0_status() & retroshelf::Cpu::StatusExl) != 0U);

    retroshelf::Cpu break_cpu;
    break_cpu.set_pc(0x8000F100U);
    assert(!break_cpu.step(0x0000000DU, &error)); // BREAK
    assert((break_cpu.cop0_cause() & (0x1FU << 2U)) ==
           (retroshelf::Cpu::ExcBreakpoint << 2U));
    assert(break_cpu.cop0_epc() == 0x8000F100U);

    // A SYSCALL in a delay slot reports the branch PC and sets Cause.BD.
    retroshelf::Cpu syscall_delay_cpu;
    syscall_delay_cpu.set_pc(0x8000F200U);
    assert(syscall_delay_cpu.step((0x04U << 26U), &error)); // BEQ zero,zero,0
    assert(!syscall_delay_cpu.step(0x0000000CU, &error)); // SYSCALL in delay slot
    assert((syscall_delay_cpu.cop0_cause() & retroshelf::Cpu::CauseBd) != 0U);
    assert((syscall_delay_cpu.cop0_cause() & (0x1FU << 2U)) ==
           (retroshelf::Cpu::ExcSyscall << 2U));
    assert(syscall_delay_cpu.cop0_epc() == 0x8000F200U);


    // EE 64-bit arithmetic and immediate shifts preserve the full GPR width.
    retroshelf::Cpu wide_cpu;
    wide_cpu.set_pc(0x8000F300U);
    wide_cpu.set_reg(8, 0x00000001FFFFFFFFULL);
    wide_cpu.set_reg(9, 2U);
    assert(wide_cpu.step((8U << 21U) | (9U << 16U) | (10U << 11U) | 0x2DU, &error)); // DADDU
    assert(wide_cpu.reg(10) == 0x0000000200000001ULL);
    assert(wide_cpu.step((8U << 21U) | (9U << 16U) | (11U << 11U) | 0x2FU, &error)); // DSUBU
    assert(wide_cpu.reg(11) == 0x00000001FFFFFFFDUll);

    retroshelf::Cpu wide_shift_cpu;
    wide_shift_cpu.set_reg(8, 0x8000000000000000ULL);
    assert(wide_shift_cpu.step((1U << 6U) | (8U << 16U) | (9U << 11U) | 0x3AU, &error)); // DSRL rt=t0, rd=t1, sa=1
    assert(wide_shift_cpu.reg(9) == 0x4000000000000000ULL);
    assert(wide_shift_cpu.step((8U << 16U) | (10U << 11U) | 0x3CU, &error)); // DSLL32 t2,t0,0
    assert(wide_shift_cpu.reg(10) == 0U); // high bit shifted out by 32
    wide_shift_cpu.set_reg(8, 0xFFFFFFFF80000000ULL);
    assert(wide_shift_cpu.step((8U << 16U) | (11U << 11U) | 0x3FU, &error)); // DSRA32 t3,t0,0
    assert(wide_shift_cpu.reg(11) == 0xFFFFFFFFFFFFFFFFULL);


    // COP0 register bank supports the common privileged setup registers used
    // by boot code, while retaining the architectural special handling for
    // Status, Cause, EPC and read-only BadVAddr.
    retroshelf::Cpu cop0_bank_cpu;
    cop0_bank_cpu.set_reg(8, 0xC1234000U);
    assert(cop0_bank_cpu.step((0x10U << 26U) | (4U << 21U) | (8U << 16U) | (10U << 11U), &error)); // MTC0 t0,EntryHi
    cop0_bank_cpu.set_reg(9, 0U);
    assert(cop0_bank_cpu.step((0x10U << 26U) | (9U << 16U) | (10U << 11U), &error)); // MFC0 t1,EntryHi
    assert(cop0_bank_cpu.reg(9) == 0xFFFFFFFFC1234000ULL);
    cop0_bank_cpu.set_reg(8, 0x12345678U);
    assert(cop0_bank_cpu.step((0x10U << 26U) | (4U << 21U) | (8U << 16U) | (9U << 11U), &error)); // MTC0 t0,Count
    assert(cop0_bank_cpu.step((0x10U << 26U) | (10U << 16U) | (9U << 11U), &error)); // MFC0 t2,Count
    assert(cop0_bank_cpu.reg(10) == 0x12345678U);

    // The CPU can fetch and execute instructions from the mapped BIOS reset-vector alias.
    retroshelf::Ps2EeMemoryMap bios_memory(64U * 1024U);
    std::vector<std::uint8_t> bios_bytes(16U, 0U);
    const std::uint32_t bios_addiu_t0 = (0x09U << 26U) | (8U << 16U) | 7U;
    const std::uint32_t bios_addiu_t1 = (0x09U << 26U) | (9U << 16U) | 5U;
    for (std::size_t i = 0; i < 4; ++i) {
        bios_bytes[i] = static_cast<std::uint8_t>(bios_addiu_t0 >> (i * 8U));
        bios_bytes[4U + i] = static_cast<std::uint8_t>(bios_addiu_t1 >> (i * 8U));
    }
    assert(bios_memory.load_bios_image(bios_bytes, &error));
    retroshelf::Cpu bios_cpu;
    bios_cpu.set_pc(0xBFC00000U);
    assert(bios_cpu.step_from_ee_memory(bios_memory, &error));
    assert(bios_cpu.reg(8) == 7U);
    assert(bios_cpu.pc() == 0xBFC00004U);
    assert(bios_cpu.step_from_ee_memory(bios_memory, &error));
    assert(bios_cpu.reg(9) == 5U);
    assert(bios_cpu.pc() == 0xBFC00008U);

    // Out-of-range fetches through the legacy RAM adapter must enter the
    // same address-exception path as fetches from the mapped EE memory.
    std::vector<std::uint8_t> short_ram(16U, 0U);
    retroshelf::Cpu ram_fetch_oob_cpu;
    ram_fetch_oob_cpu.set_pc(0x1020U);
    assert(!ram_fetch_oob_cpu.step_from_ram(short_ram, 0x1000U, &error));
    assert(ram_fetch_oob_cpu.cop0_bad_vaddr() == 0x1020U);
    assert(ram_fetch_oob_cpu.cop0_epc() == 0x1020U);
    assert((ram_fetch_oob_cpu.cop0_cause() & (0x1FU << 2U)) ==
           (retroshelf::Cpu::ExcAddressErrorLoad << 2U));
    assert(ram_fetch_oob_cpu.pc() == retroshelf::Cpu::GeneralExceptionVector);

    retroshelf::Cpu ram_fetch_below_cpu;
    ram_fetch_below_cpu.set_pc(0x0FFCU);
    assert(!ram_fetch_below_cpu.step_from_ram(short_ram, 0x1000U, &error));
    assert(ram_fetch_below_cpu.cop0_bad_vaddr() == 0x0FFCU);
    assert(ram_fetch_below_cpu.cop0_epc() == 0x0FFCU);
    assert((ram_fetch_below_cpu.cop0_cause() & (0x1FU << 2U)) ==
           (retroshelf::Cpu::ExcAddressErrorLoad << 2U));
    assert(ram_fetch_below_cpu.pc() == retroshelf::Cpu::GeneralExceptionVector);

    std::cout << "CPU boot execution tests passed\\n";
}
