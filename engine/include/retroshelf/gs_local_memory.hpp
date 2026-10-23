#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace retroshelf {

// Byte-addressable backing store for the PS2 Graphics Synthesizer local memory.
// The GS exposes 4 MiB of local memory; pixel formats and transfer registers are
// interpreted by higher layers, not by this raw storage primitive.
class GsLocalMemory {
public:
    static constexpr std::size_t SizeBytes = 4U * 1024U * 1024U;

    GsLocalMemory();

    // Read/write byte ranges with GS local-memory address wrapping.
    // Empty ranges succeed. Oversized ranges are rejected to keep work bounded.
    bool write(std::uint32_t address, const std::uint8_t* data, std::size_t size);
    bool read(std::uint32_t address, std::uint8_t* destination, std::size_t size) const;
    bool fill(std::uint32_t address, std::uint8_t value, std::size_t size);
    void reset();

    std::uint8_t read_byte(std::uint32_t address) const;
    bool write_byte(std::uint32_t address, std::uint8_t value);
    const std::vector<std::uint8_t>& bytes() const;

private:
    std::vector<std::uint8_t> memory_;
};

} // namespace retroshelf
