#pragma once

#include "retroshelf/ee_bus.hpp"
#include "retroshelf/ee_cpu.hpp"

#include <cstdint>
#include <string>

namespace retroshelf::ee {

// Small integration harness: fetches instruction words from the project's
// own RAM bus and passes them to the project's own CPU. It is not BIOS boot.
class EeMachine {
public:
    EeMachine();
    void reset();
    bool load_word(std::uint32_t address, std::uint32_t word);
    bool step(std::string* error = nullptr);
    EeCpu& cpu();
    const EeCpu& cpu() const;
    EeBus& bus();
    const EeBus& bus() const;
private:
    EeCpu cpu_;
    EeBus bus_;
};

} // namespace retroshelf::ee
