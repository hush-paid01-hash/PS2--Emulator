#pragma once

#include "retroshelf/ee_bus.hpp"
#include "retroshelf/ee_hardware.hpp"
#include "retroshelf/ee_cpu.hpp"
#include "retroshelf/ee_scheduler.hpp"
#include "retroshelf/ee_system_bus.hpp"

#include <cstdint>
#include <string>

namespace retroshelf::ee {

// Integration harness for synthetic tests. One instruction currently advances
// one placeholder cycle; hardware-accurate timing will replace this later.
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
    EeHardware& hardware();
    const EeHardware& hardware() const;
    EeScheduler& scheduler();
    const EeScheduler& scheduler() const;
private:
    EeCpu cpu_;
    EeBus bus_;
    EeHardware hardware_;
    EeSystemBus system_bus_;
    EeScheduler scheduler_;
};

} // namespace retroshelf::ee
