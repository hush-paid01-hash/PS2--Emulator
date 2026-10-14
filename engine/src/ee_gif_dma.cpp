#include "retroshelf/ee_gif_dma.hpp"
#include <limits>

namespace retroshelf::ee {
void EeGifDma::reset() { chcr_ = madr_ = qwc_ = tadr_ = 0; last_error_.clear(); }
void EeGifDma::set_transfer_handler(TransferHandler handler) { handler_ = std::move(handler); }
bool EeGifDma::handles(std::uint32_t address) const {
    return address == Chcr || address == Madr || address == Qwc || address == Tadr;
}
bool EeGifDma::read32(std::uint32_t address, std::uint32_t& value) const {
    switch (address) {
        case Chcr: value = chcr_; return true;
        case Madr: value = madr_; return true;
        case Qwc: value = qwc_; return true;
        case Tadr: value = tadr_; return true;
        default: return false;
    }
}
bool EeGifDma::write32(std::uint32_t address, std::uint32_t value) {
    switch (address) {
        case Chcr:
            // Only DIR=1 (memory to GIF), normal mode (MOD=0) is implemented.
            if ((value & 1U) == 0U || (value & (3U << 2U)) != 0U) {
                last_error_ = "GIF DMAC supports memory-to-GIF normal mode only";
                chcr_ = value & ~Str;
                return false;
            }
            chcr_ = value;
            if ((chcr_ & Str) != 0U) {
                const bool ok = start(&last_error_);
                chcr_ &= ~Str; // synchronous prototype completion
                return ok;
            }
            return true;
        case Madr: madr_ = value & 0x7FFFFFF0U; return true;
        case Qwc: qwc_ = value & 0xFFFFU; return true;
        case Tadr: tadr_ = value & 0x7FFFFFF0U; return true;
        default: return false;
    }
}
bool EeGifDma::start(std::string* error) {
    if (qwc_ == 0U) { last_error_ = "GIF DMAC QWC must be nonzero"; if (error) *error = last_error_; return false; }
    if (!handler_) { last_error_ = "GIF DMAC has no GS transfer handler"; if (error) *error = last_error_; return false; }
    if (static_cast<std::uint64_t>(madr_) + static_cast<std::uint64_t>(qwc_) * 16U >
        static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) + 1ULL) {
        last_error_ = "GIF DMAC transfer address overflows 32-bit address space";
        if (error) *error = last_error_;
        return false;
    }
    if (!handler_(madr_, qwc_, error)) {
        last_error_ = error && !error->empty() ? *error : "GIF DMAC transfer failed";
        return false;
    }
    madr_ += qwc_ * 16U;
    qwc_ = 0;
    last_error_.clear();
    if (error) error->clear();
    return true;
}
const std::string& EeGifDma::last_error() const { return last_error_; }
} // namespace retroshelf::ee
