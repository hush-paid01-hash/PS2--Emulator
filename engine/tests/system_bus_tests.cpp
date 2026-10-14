#include "retroshelf/system_bus.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

int main() {
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) {
            std::cerr << "FAIL: " << label << "\n";
            ++failures;
        } else {
            std::cout << "PASS: " << label << "\n";
        }
    };

    std::vector<std::uint8_t> ram(32, 0);
    retroshelf::SystemBus bus(ram);
    const std::vector<std::uint8_t> last_qword(16, 0xA5);
    std::string error;
    check(bus.ram_size() == 32, "bus reports its mapped RAM size");
    check(bus.write(16, last_qword, &error) && error.empty(),
          "write ending exactly at the RAM boundary succeeds");

    std::vector<std::uint8_t> readback;
    check(bus.read(16, 16, readback, &error) && readback == last_qword,
          "read returns the written bytes without changing their order");
    check(bus.read(32, 0, readback, &error) && readback.empty(),
          "zero-byte access at the end boundary succeeds");
    check(!bus.read(32, 1, readback, &error) && readback.empty() &&
              error.find("outside EE RAM") != std::string::npos,
          "read beyond the mapped boundary fails and clears output");

    const auto before = ram;
    check(!bus.write(31, {0x11, 0x22}, &error) && ram == before &&
              error.find("outside EE RAM") != std::string::npos,
          "out-of-range write fails without partially modifying RAM");
    check(!bus.read(UINT32_MAX, 2, readback, &error),
          "large address cannot overflow range validation");

    return failures == 0 ? 0 : 1;
}
