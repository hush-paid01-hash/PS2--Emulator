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
    assert(mult_cpu.lo() == 0xFFFFFFFAU);
    assert(mult_cpu.hi() == 0xFFFFFFFFU);

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

    std::cout << "CPU boot execution tests passed\\n";
}
