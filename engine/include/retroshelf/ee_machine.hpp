#pragma once

#include "retroshelf/ee_bus.hpp"
#include "retroshelf/ee_hardware.hpp"
#include "retroshelf/ee_cpu.hpp"
#include "retroshelf/ee_scheduler.hpp"
#include "retroshelf/ee_system_bus.hpp"
#include "retroshelf/disc_image.hpp"
#include "retroshelf/ps2_elf.hpp"

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
    bool load_elf(const retroshelf::media::Ps2ElfImage& image, std::string* error = nullptr);
    bool load_ps2_disc(const std::string& path, retroshelf::media::Ps2BootInfo* boot = nullptr,
                       std::string* error = nullptr);
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
