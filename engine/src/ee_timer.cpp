#include "retroshelf/ee_timer.hpp"

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

bool EeTimerBlock::read(std::uint32_t offset, std::size_t size,
                        std::vector<std::uint8_t>& data,
                        std::string* error) const {
    if (size != 4 || (offset & 3U) != 0U || offset >= RegisterSize) {
        if (error) *error = "EE timers support aligned 32-bit register reads only";
        data.clear();
        return false;
    }
    const std::uint32_t channel = offset / ChannelStride;
    const std::uint32_t reg = offset % ChannelStride;
    if (channel >= ChannelCount) {
        if (error) *error = "EE timer channel is out of range";
        data.clear();
        return false;
    }

    const Timer& timer = timers_[channel];
    if (reg == CountOffset) encode_u32(timer.count, data);
    else if (reg == ModeOffset) encode_u32(timer.mode, data);
    else if (reg == CompareOffset) encode_u32(timer.compare, data);
    else if (reg == HoldOffset) encode_u32(timer.hold, data);
    else {
        if (error) *error = "Read from an unimplemented EE timer register";
        data.clear();
        return false;
    }
    if (error) error->clear();
    return true;
}

bool EeTimerBlock::write(std::uint32_t offset,
                         const std::vector<std::uint8_t>& data,
                         std::string* error) {
    if (data.size() != 4 || (offset & 3U) != 0U || offset >= RegisterSize) {
        if (error) *error = "EE timers support aligned 32-bit register writes only";
        return false;
    }
    const std::uint32_t channel = offset / ChannelStride;
    const std::uint32_t reg = offset % ChannelStride;
    if (channel >= ChannelCount) {
        if (error) *error = "EE timer channel is out of range";
        return false;
    }

    Timer& timer = timers_[channel];
    const std::uint32_t value = decode_u32(data);
    if (reg == CountOffset) {
        timer.count = static_cast<std::uint16_t>(value);
        timer.prescale_remainder = 0;
    } else if (reg == ModeOffset) {
        // EQUF and OVFF are write-one-to-clear status bits. Control bits are
        // ordinary read/write fields; reserved bits are kept clear.
        const std::uint32_t flags = timer.mode &
            (ModeCompareFlag | ModeOverflowFlag);
        const std::uint32_t clear = value &
            (ModeCompareFlag | ModeOverflowFlag);
        const std::uint32_t controls = value & 0x3FFU;
        timer.mode = controls | (flags & ~clear);
        timer.prescale_remainder = 0;
    } else if (reg == CompareOffset) {
        timer.compare = static_cast<std::uint16_t>(value);
    } else if (reg == HoldOffset) {
        timer.hold = static_cast<std::uint16_t>(value);
    } else {
        if (error) *error = "Write to an unimplemented EE timer register";
        return false;
    }
    if (error) error->clear();
    return true;
}

void EeTimerBlock::advance_cycles(std::uint64_t cycles) {
    for (Timer& timer : timers_) {
        if ((timer.mode & ModeCountEnable) == 0U ||
            (timer.mode & ModeGate) != 0U) continue;

        const std::uint32_t clock = timer.mode & ModeClockMask;
        if (clock == 3U) continue; // H/V-blank clock requires external events.
        const std::uint64_t divisor = clock == 0U ? 1U : (clock == 1U ? 16U : 256U);
        const std::uint64_t total = timer.prescale_remainder + cycles;
        std::uint64_t ticks = total / divisor;
        timer.prescale_remainder = total % divisor;

        while (ticks-- != 0U) {
            const std::uint16_t previous = timer.count;
            timer.count = static_cast<std::uint16_t>(timer.count + 1U);
            if (timer.count == 0U) {
                timer.mode |= ModeOverflowFlag;
            }
            if (timer.count == timer.compare) {
                timer.mode |= ModeCompareFlag;
                if ((timer.mode & ModeZeroReturn) != 0U) timer.count = 0U;
            }
            (void)previous;
        }
    }
}

std::uint16_t EeTimerBlock::count(std::uint32_t channel) const {
    return channel < ChannelCount ? timers_[channel].count : 0U;
}
std::uint16_t EeTimerBlock::compare(std::uint32_t channel) const {
    return channel < ChannelCount ? timers_[channel].compare : 0U;
}
std::uint32_t EeTimerBlock::mode(std::uint32_t channel) const {
    return channel < ChannelCount ? timers_[channel].mode : 0U;
}
std::uint32_t EeTimerBlock::pending_interrupts() const {
    std::uint32_t pending = 0;
    for (std::uint32_t i = 0; i < ChannelCount; ++i) {
        const std::uint32_t m = timers_[i].mode;
        if (((m & ModeCompareEnable) != 0U && (m & ModeCompareFlag) != 0U) ||
            ((m & ModeOverflowEnable) != 0U && (m & ModeOverflowFlag) != 0U))
            pending |= 1U << i;
    }
    return pending;
}

} // namespace retroshelf
