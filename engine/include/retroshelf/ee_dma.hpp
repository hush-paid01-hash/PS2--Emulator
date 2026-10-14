#pragma once
#include "retroshelf/ee_bus.hpp"
#include <cstddef>
#include <cstdint>

namespace retroshelf::ee {

// First-stage memory-to-memory DMA helper for deterministic tests.
// This is not yet the PS2 DMAC channel/register implementation.
class EeDma {
public:
    bool transfer(EeBus& bus, std::uint32_t source, std::uint32_t destination, std::size_t bytes) const;
};
} // namespace retroshelf::ee
