#include "retroshelf/ee_machine.hpp"

#include <sstream>
#include <iomanip>

namespace retroshelf::ee {

EeMachine::EeMachine() { reset(); }
void EeMachine::reset() { cpu_.reset(); bus_.reset(); }

bool EeMachine::load_word(std::uint32_t address, std::uint32_t word) {
    return bus_.write32(address, word);
}

bool EeMachine::step(std::string* error) {
    const auto pc = cpu_.pc();
    if (pc > 0xFFFFFFFFULL) {
        if (error) *error = "EE instruction fetch address exceeds 32-bit prototype bus";
        return false;
    }
    std::uint32_t instruction = 0;
    if (!bus_.read32(static_cast<std::uint32_t>(pc), instruction)) {
        if (error) {
            std::ostringstream out;
            out << "EE instruction fetch failed at PC=0x" << std::hex << pc;
            *error = out.str();
        }
        return false;
    }
    return cpu_.step(instruction, error);
}

EeCpu& EeMachine::cpu() { return cpu_; }
const EeCpu& EeMachine::cpu() const { return cpu_; }
EeBus& EeMachine::bus() { return bus_; }
const EeBus& EeMachine::bus() const { return bus_; }

} // namespace retroshelf::ee
