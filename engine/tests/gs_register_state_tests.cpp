#include "retroshelf/gs_register_state.hpp"

#include <cstdint>
#include <iostream>

int main() {
    int failures = 0;
    const auto check = [&failures](bool condition, const char* message) {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        } else {
            std::cout << "PASS: " << message << '\n';
        }
    };

    retroshelf::GsRegisterState gs;
    std::uint64_t value = 99;
    check(gs.read(0x00, value) && value == 0,
          "registers begin at zero after initialization");
    check(gs.write(0x00, 0x123456789ABCDEF0ULL),
          "valid GS register accepts a 64-bit write");
    check(gs.read(0x00, value) && value == 0x123456789ABCDEF0ULL,
          "register read returns the last written value");
    check(gs.write(0x7F, 0xFEDCBA9876543210ULL) &&
              gs.read(0x7F, value) && value == 0xFEDCBA9876543210ULL,
          "last 7-bit register address is accessible");
    check(!gs.read(0x80, value), "out-of-range register read is rejected");
    check(!gs.write(0x80, 1), "out-of-range register write is rejected");
    check(gs.write_count() == 2, "only successful writes are counted");
    gs.reset();
    check(gs.read(0x00, value) && value == 0 && gs.write_count() == 0,
          "reset clears registers and write counter");

    return failures == 0 ? 0 : 1;
}
