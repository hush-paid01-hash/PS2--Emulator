#pragma once

#include "retroshelf/elf32_mips.hpp"
#include "retroshelf/system_bus.hpp"

#include <cstdint>
#include <string>

namespace retroshelf {

// Copies PT_LOAD data and zero-fills BSS through the bounded EE RAM bus.
// Addresses are direct offsets in this simplified bus; PS2 KSEG aliases and
// the complete hardware memory map are intentionally not implemented.
bool load_elf32_into_ram(const Elf32MipsImage& image,
                         std::uint32_t ram_address_base,
                         SystemBus& bus,
                         std::uint32_t& entry_point,
                         std::string* error = nullptr);

} // namespace retroshelf
