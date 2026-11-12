#include "retroshelf/ee_interrupt_controller.hpp"

namespace retroshelf {
namespace {

void encode_u32(std::uint32_t value, std::vector<std::uint8_t>& data) {
    data.resize(4);
    for (std::size_t i = 0; i < 4; ++i)
        data[i] = static_cast<std::uint8_t>((value >> (i * 8U)) & 0xFFU);
}

std::uint32_t decode_u32(const std::vector<std::uint8_t>& data) {
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < 4; ++i)
        value |= static_cast<std::uint32_t>(data[i]) << (i * 8U);
    return value;
}

} // namespace

bool EeInterruptController::read(std::uint32_t offset, std::size_t size,
                                 std::vector<std::uint8_t>& data,
                                 std::string* error) const {
    if (size != 4 || (offset & 3U) != 0U) {
        if (error) *error = "EE INTC supports aligned 32-bit register reads only";
        data.clear();
        return false;
    }

    if (offset == StatOffset) {
        encode_u32(status_, data);
    } else if (offset == MaskOffset) {
        encode_u32(mask_, data);
    } else {
        if (error) *error = "Read from an unimplemented EE INTC register";
        data.clear();
        return false;
    }

    if (error) error->clear();
    return true;
}

bool EeInterruptController::write(std::uint32_t offset,
                                  const std::vector<std::uint8_t>& data,
                                  std::string* error) {
    if (data.size() != 4 || (offset & 3U) != 0U) {
        if (error) *error = "EE INTC supports aligned 32-bit register writes only";
        return false;
    }

    const std::uint32_t value = decode_u32(data);
    if (offset == StatOffset) {
        // INTC_STAT is write-one-to-clear.
        status_ &= ~value;
    } else if (offset == MaskOffset) {
        // INTC_MASK writes toggle the selected mask bits.
        mask_ ^= value;
    } else {
        if (error) *error = "Write to an unimplemented EE INTC register";
        return false;
    }

    if (error) error->clear();
    return true;
}

bool EeInterruptController::raise_interrupt(std::uint32_t line) {
    if (line >= InterruptLineCount) return false;
    status_ |= (std::uint32_t{1} << line);
    return true;
}

bool EeInterruptController::clear_interrupt(std::uint32_t line) {
    if (line >= InterruptLineCount) return false;
    status_ &= ~(std::uint32_t{1} << line);
    return true;
}

std::uint32_t EeInterruptController::status() const { return status_; }
std::uint32_t EeInterruptController::mask() const { return mask_; }
std::uint32_t EeInterruptController::active_pending() const { return status_ & mask_; }

} // namespace retroshelf
