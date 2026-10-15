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

    std::cout << "CPU boot execution tests passed\n";
}
