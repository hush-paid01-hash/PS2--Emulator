#include "retroshelf/gs_local_memory.hpp"

#include <algorithm>

namespace retroshelf {

GsLocalMemory::GsLocalMemory() : memory_(SizeBytes, 0) {}

bool GsLocalMemory::write(std::uint32_t address, const std::uint8_t* data, std::size_t size) {
    if (size > SizeBytes || (size != 0U && data == nullptr)) return false;
    std::size_t offset = static_cast<std::size_t>(address) % SizeBytes;
    for (std::size_t i = 0; i < size; ++i) {
        memory_[offset] = data[i];
        if (++offset == SizeBytes) offset = 0;
    }
    return true;
}

bool GsLocalMemory::read(std::uint32_t address, std::uint8_t* destination, std::size_t size) const {
    if (size > SizeBytes || (size != 0U && destination == nullptr)) return false;
    std::size_t offset = static_cast<std::size_t>(address) % SizeBytes;
    for (std::size_t i = 0; i < size; ++i) {
        destination[i] = memory_[offset];
        if (++offset == SizeBytes) offset = 0;
    }
    return true;
}

bool GsLocalMemory::fill(std::uint32_t address, std::uint8_t value, std::size_t size) {
    if (size > SizeBytes) return false;
    std::size_t offset = static_cast<std::size_t>(address) % SizeBytes;
    for (std::size_t i = 0; i < size; ++i) {
        memory_[offset] = value;
        if (++offset == SizeBytes) offset = 0;
    }
    return true;
}

void GsLocalMemory::reset() {
    std::fill(memory_.begin(), memory_.end(), 0);
}

std::uint8_t GsLocalMemory::read_byte(std::uint32_t address) const {
    return memory_[static_cast<std::size_t>(address) % SizeBytes];
}

bool GsLocalMemory::write_byte(std::uint32_t address, std::uint8_t value) {
    memory_[static_cast<std::size_t>(address) % SizeBytes] = value;
    return true;
}

const std::vector<std::uint8_t>& GsLocalMemory::bytes() const { return memory_; }

} // namespace retroshelf
