#include "retroshelf/gs_register_state.hpp"

namespace retroshelf {

GsRegisterState::GsRegisterState() { reset(); }

bool GsRegisterState::write(std::uint8_t address, std::uint64_t value) {
    if (static_cast<std::size_t>(address) >= RegisterCount) {
        return false;
    }
    registers_[address] = value;
    ++write_count_;
    return true;
}

bool GsRegisterState::read(std::uint8_t address, std::uint64_t& value) const {
    if (static_cast<std::size_t>(address) >= RegisterCount) {
        return false;
    }
    value = registers_[address];
    return true;
}

void GsRegisterState::reset() {
    registers_.fill(0);
    write_count_ = 0;
}

std::uint64_t GsRegisterState::write_count() const { return write_count_; }

} // namespace retroshelf
