#include "retroshelf/ee_machine.hpp"

#include <iomanip>
#include <limits>
#include <sstream>

namespace retroshelf::ee {

EeMachine::EeMachine() : system_bus_(bus_, hardware_) { reset(); }
void EeMachine::reset() { cpu_.reset(); bus_.reset(); hardware_.reset(); scheduler_.reset(); }
bool EeMachine::load_word(std::uint32_t address, std::uint32_t word) {
    return bus_.write32(address, word);
}
bool EeMachine::load_elf(const retroshelf::media::Ps2ElfImage& image, std::string* error) {
    if (!image.valid || image.segments.empty()) {
        if (error) *error = "Cannot load an invalid or empty ELF image";
        return false;
    }
    // Validate every segment first so a bad later segment cannot leave RAM partially modified.
    for (const auto& segment : image.segments) {
        const std::uint64_t end = static_cast<std::uint64_t>(segment.virtual_address) + segment.memory_size;
        if (segment.memory_size != segment.bytes.size() || end > EeBus::RamSize) {
            if (error) *error = "ELF segment does not fit the prototype physical RAM map";
            return false;
        }
    }
    bool entry_mapped = false;
    for (const auto& segment : image.segments) {
        const auto begin = static_cast<std::uint64_t>(segment.virtual_address);
        const auto end = begin + segment.memory_size;
        if (image.entry_point >= begin && image.entry_point < end) entry_mapped = true;
    }
    if (!entry_mapped || image.entry_point >= EeBus::RamSize) {
        if (error) *error = "ELF entry point is not addressable in prototype physical RAM";
        return false;
    }
    for (const auto& segment : image.segments) {
        for (std::size_t i = 0; i < segment.bytes.size(); ++i) {
            if (!bus_.write8(segment.virtual_address + static_cast<std::uint32_t>(i), segment.bytes[i])) {
                if (error) *error = "Unexpected RAM write failure while loading ELF segment";
                return false;
            }
        }
    }
    cpu_.set_pc(image.entry_point);
    if (error) error->clear();
    return true;
}
bool EeMachine::step(std::string* error) {
    const auto pc = cpu_.pc();
    if (pc > 0xFFFFFFFFULL) {
        if (error) *error = "EE instruction fetch address exceeds 32-bit prototype bus";
        return false;
    }
    std::uint32_t instruction = 0;
    if (!system_bus_.read32(static_cast<std::uint32_t>(pc), instruction)) {
        if (error) {
            std::ostringstream out;
            out << "EE instruction fetch failed at PC=0x" << std::hex << pc;
            *error = out.str();
        }
        return false;
    }
    const auto read_word = [this](std::uint32_t address, std::uint32_t& value) {
        return system_bus_.read32(address, value);
    };
    const auto write_word = [this](std::uint32_t address, std::uint32_t value) {
        return system_bus_.write32(address, value);
    };
    if (!cpu_.step(instruction, error, read_word, write_word)) return false;
    // Temporary one-cycle model for plumbing tests, not PS2 timing accuracy.
    scheduler_.advance(1);
    hardware_.advance(1);
    return true;
}
EeCpu& EeMachine::cpu() { return cpu_; }
const EeCpu& EeMachine::cpu() const { return cpu_; }
EeBus& EeMachine::bus() { return bus_; }
const EeBus& EeMachine::bus() const { return bus_; }
EeHardware& EeMachine::hardware() { return hardware_; }
const EeHardware& EeMachine::hardware() const { return hardware_; }
EeScheduler& EeMachine::scheduler() { return scheduler_; }
const EeScheduler& EeMachine::scheduler() const { return scheduler_; }

} // namespace retroshelf::ee
