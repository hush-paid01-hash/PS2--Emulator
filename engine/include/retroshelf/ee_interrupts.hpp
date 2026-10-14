#pragma once

#include <cstdint>

namespace retroshelf::ee {

// Minimal testable interrupt-pending/mask model. Hardware-specific routing,
// priority, and exception delivery remain to be implemented.
class EeInterrupts {
public:
    void reset();
    void raise(std::uint32_t source_bit);
    void clear(std::uint32_t source_bit);
    void set_mask(std::uint32_t mask);
    std::uint32_t pending() const;
    std::uint32_t mask() const;
    bool has_enabled_pending() const;
private:
    std::uint32_t pending_{0};
    std::uint32_t mask_{0};
};

} // namespace retroshelf::ee
