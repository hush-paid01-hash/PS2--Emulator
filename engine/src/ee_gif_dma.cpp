#include "retroshelf/ee_gif_dma.hpp"

#include <exception>
#include <limits>
#include <utility>

namespace retroshelf {
namespace {

constexpr std::uint32_t StartBit = 1U << 8U;
constexpr std::uint32_t DirectionBit = 1U;
constexpr std::uint32_t ModeMask = 3U << 2U;
constexpr std::uint32_t AspMask = 3U << 4U;
constexpr std::uint32_t ChainMode = 1U << 2U;
constexpr std::uint32_t TagTransferEnable = 1U << 6U;
constexpr std::uint32_t TagInterruptEnable = 1U << 7U;

enum class TagId : std::uint8_t {
    Refe = 0,
    Cnt = 1,
    Next = 2,
    Ref = 3,
    Refs = 4,
    Call = 5,
    Ret = 6,
    End = 7
};

std::uint64_t read_u64(const std::vector<std::uint8_t>& ram, std::size_t address) {
    std::uint64_t value = 0;
    for (std::size_t byte = 0; byte < 8; ++byte) {
        value |= static_cast<std::uint64_t>(ram[address + byte]) << (byte * 8U);
    }
    return value;
}

} // namespace

EeGifDma::EeGifDma(std::vector<std::uint8_t>& ee_ram, PayloadHandler payload_handler,
                   InterruptHandler interrupt_handler)
    : owned_bus_(std::make_unique<SystemBus>(ee_ram)), bus_(*owned_bus_),
      payload_handler_(std::move(payload_handler)),
      interrupt_handler_(std::move(interrupt_handler)) {}

EeGifDma::EeGifDma(SystemBus& bus, PayloadHandler payload_handler,
                   InterruptHandler interrupt_handler)
    : bus_(bus), payload_handler_(std::move(payload_handler)),
      interrupt_handler_(std::move(interrupt_handler)) {}

EeGifDma::State EeGifDma::state() const { return state_; }
const std::string& EeGifDma::error() const { return error_; }

void EeGifDma::interrupt(Interrupt reason) {
    if (interrupt_handler_) {
        interrupt_handler_(reason);
    }
}

bool EeGifDma::fail(const std::string& message) {
    error_ = message;
    state_ = State::Error;
    registers.chcr &= ~StartBit;
    registers.qwc = 0;
    interrupt(Interrupt::Error);
    return false;
}

bool EeGifDma::transfer(std::uint32_t address, std::uint32_t qwc) {
    constexpr std::size_t qword_size = QwordSize;
    const std::size_t count = static_cast<std::size_t>(qwc);
    if (count > std::numeric_limits<std::size_t>::max() / qword_size) {
        return fail("GIF DMA payload size overflows");
    }
    const std::size_t size = count * qword_size;
    if (size > MaxTransferBytes) {
        return fail("GIF DMA payload exceeds the transfer limit");
    }
    if (size > std::numeric_limits<std::uint32_t>::max() - address) {
        return fail("GIF DMA payload address overflows");
    }
    if ((address & (QwordSize - 1U)) != 0U) {
        return fail("GIF DMA payload address is unaligned");
    }

    registers.madr = address;
    registers.qwc = qwc;
    std::vector<std::uint8_t> payload;
    std::string bus_error;
    if (!bus_.read(address, size, payload, &bus_error)) {
        return fail("GIF DMA payload read failed: " + bus_error);
    }
    if (size != 0 && !payload_handler_) {
        return fail("GIF DMA has no payload handler");
    }
    if (size != 0) {
        try {
            payload_handler_(payload);
        } catch (const std::exception& exception) {
            return fail("GIF DMA payload handler failed: " + std::string(exception.what()));
        } catch (...) {
            return fail("GIF DMA payload handler failed");
        }
    }
    registers.madr = address + static_cast<std::uint32_t>(size);
    registers.qwc = 0;
    return true;
}

bool EeGifDma::start() {
    error_.clear();
    if ((registers.chcr & StartBit) == 0U) {
        return fail("GIF DMA start bit is not set");
    }
    state_ = State::Running;
    registers.asp = static_cast<std::uint8_t>((registers.chcr & AspMask) >> 4U);
    if (registers.asp > 2U) {
        return fail("GIF DMA address stack pointer is invalid");
    }

    if ((registers.chcr & DirectionBit) != 0U) {
        return fail("GIF DMA direction must be EE RAM to GIF");
    }

    const std::uint32_t mode = registers.chcr & ModeMask;
    if (mode == 0U) {
        if (!transfer(registers.madr, registers.qwc)) {
            return false;
        }
        registers.chcr &= ~StartBit;
        state_ = State::Completed;
        interrupt(Interrupt::Completion);
        return true;
    }
    if (mode != ChainMode) {
        return fail("GIF DMA supports normal and chain modes only");
    }

    std::size_t tags_processed = 0;
    std::size_t bytes_transferred = 0;
    while (true) {
        if (tags_processed++ >= MaxChainTags) {
            return fail("GIF DMA chain exceeded the tag limit");
        }
        const std::uint32_t tag_address = registers.tadr;
        if ((tag_address & (QwordSize - 1U)) != 0U) {
            return fail("GIF DMA tag address is unaligned");
        }
        std::vector<std::uint8_t> tag;
        std::string bus_error;
        if (!bus_.read(tag_address, QwordSize, tag, &bus_error)) {
            return fail("GIF DMA tag read failed: " + bus_error);
        }

        const std::uint64_t tag_low = read_u64(tag, 0);
        const std::uint64_t tag_high = read_u64(tag, 8);
        const auto qwc = static_cast<std::uint32_t>(tag_low & 0xFFFFU);
        const auto id = static_cast<TagId>((tag_low >> 28U) & 7U);
        const bool tag_irq = ((tag_low >> 31U) & 1U) != 0U;
        const bool scratchpad = ((tag_high >> 31U) & 1U) != 0U;
        const auto tag_address_field = static_cast<std::uint32_t>(tag_high & 0x7FFFFFFFU);

        if (scratchpad) {
            return fail("GIF DMA scratchpad addresses are not supported");
        }
        if ((tag_address_field & (QwordSize - 1U)) != 0U &&
            (id == TagId::Refe || id == TagId::Next ||
             id == TagId::Ref || id == TagId::Refs || id == TagId::Call)) {
            return fail("GIF DMA tag target is not qword-aligned");
        }

        const std::uint64_t inline_address_wide =
            static_cast<std::uint64_t>(tag_address) + QwordSize;
        const std::uint64_t after_inline_wide =
            inline_address_wide + static_cast<std::uint64_t>(qwc) * QwordSize;
        if (after_inline_wide > std::numeric_limits<std::uint32_t>::max()) {
            return fail("GIF DMA chain address overflows");
        }
        const auto inline_address = static_cast<std::uint32_t>(inline_address_wide);
        const auto after_inline = static_cast<std::uint32_t>(after_inline_wide);
        std::uint32_t payload_address = inline_address;
        std::uint32_t next_tag = after_inline;
        bool terminate = false;

        switch (id) {
        case TagId::Refe:
            payload_address = tag_address_field;
            next_tag = tag_address + static_cast<std::uint32_t>(QwordSize);
            terminate = true;
            break;
        case TagId::Cnt:
            break;
        case TagId::Next:
            next_tag = tag_address_field;
            break;
        case TagId::Ref:
        case TagId::Refs:
            payload_address = tag_address_field;
            next_tag = inline_address;
            break;
        case TagId::Call:
            if (registers.asp >= 2U) {
                return fail("GIF DMA call stack overflow");
            }
            if (registers.asp == 0U) {
                registers.asr0 = after_inline;
            } else {
                registers.asr1 = after_inline;
            }
            ++registers.asp;
            registers.chcr = (registers.chcr & ~AspMask) |
                             (static_cast<std::uint32_t>(registers.asp) << 4U);
            next_tag = tag_address_field;
            break;
        case TagId::Ret:
            if (registers.asp == 0U) {
                return fail("GIF DMA return stack underflow");
            }
            --registers.asp;
            registers.chcr = (registers.chcr & ~AspMask) |
                             (static_cast<std::uint32_t>(registers.asp) << 4U);
            if (registers.asp == 0U) {
                next_tag = registers.asr0;
            } else {
                next_tag = registers.asr1;
            }
            break;
        case TagId::End:
            terminate = true;
            break;
        }

        const bool transfer_tag = (registers.chcr & TagTransferEnable) != 0U;
        const std::size_t tag_bytes = transfer_tag ? QwordSize : 0U;
        const std::size_t payload_bytes = static_cast<std::size_t>(qwc) * QwordSize;
        if (bytes_transferred > MaxTransferBytes ||
            tag_bytes > MaxTransferBytes - bytes_transferred ||
            payload_bytes > MaxTransferBytes - bytes_transferred - tag_bytes) {
            return fail("GIF DMA chain exceeded the transfer limit");
        }
        if (transfer_tag) {
            if (!payload_handler_) {
                return fail("GIF DMA has no payload handler for transfer tag");
            }
            try {
                payload_handler_(tag);
            } catch (const std::exception& exception) {
                return fail("GIF DMA tag handler failed: " + std::string(exception.what()));
            } catch (...) {
                return fail("GIF DMA tag handler failed");
            }
        }
        if (!transfer(payload_address, qwc)) {
            return false;
        }
        bytes_transferred += tag_bytes + payload_bytes;
        registers.tadr = next_tag;

        if (tag_irq && (registers.chcr & TagInterruptEnable) != 0U) {
            registers.chcr &= ~StartBit;
            state_ = State::Completed;
            interrupt(Interrupt::Tag);
            return true;
        }
        if (terminate) {
            registers.chcr &= ~StartBit;
            state_ = State::Completed;
            interrupt(Interrupt::Completion);
            return true;
        }
    }
}

} // namespace retroshelf
