#include "retroshelf/ee_dma.hpp"
#include <limits>
#include <vector>

namespace retroshelf::ee {
bool EeDma::transfer(EeBus& bus, std::uint32_t source, std::uint32_t destination, std::size_t bytes) const {
    const auto size = static_cast<std::uint64_t>(bus.ram_size());
    const auto src = static_cast<std::uint64_t>(source);
    const auto dst = static_cast<std::uint64_t>(destination);
    if (src > size || dst > size || bytes > size - src || bytes > size - dst) return false;
    std::vector<std::uint8_t> staging(bytes);
    for (std::size_t i = 0; i < bytes; ++i) {
        if (!bus.read8(static_cast<std::uint32_t>(src + i), staging[i])) return false;
    }
    for (std::size_t i = 0; i < bytes; ++i) {
        if (!bus.write8(static_cast<std::uint32_t>(dst + i), staging[i])) return false;
    }
    return true;
}
} // namespace retroshelf::ee
