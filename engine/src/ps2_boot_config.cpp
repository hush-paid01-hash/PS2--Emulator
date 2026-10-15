#include "retroshelf/ps2_boot_config.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace retroshelf {
namespace {
std::string trim(std::string value) {
    const auto first = std::find_if_not(value.begin(), value.end(),
        [](unsigned char c) { return std::isspace(c) != 0; });
    if (first == value.end()) return {};
    const auto last = std::find_if_not(value.rbegin(), value.rend(),
        [](unsigned char c) { return std::isspace(c) != 0; }).base();
    return std::string(first, last);
}
std::string upper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return value;
}
}

bool parse_system_cnf(const std::string& contents, std::string& executable_path,
                      std::string* error) {
    executable_path.clear();
    std::string boot2;
    std::string boot;
    std::istringstream input(contents);
    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        const auto equal = line.find('=');
        if (equal == std::string::npos) continue;
        const auto key = upper(trim(line.substr(0, equal)));
        if (key != "BOOT2" && key != "BOOT") continue;
        std::string value = trim(line.substr(equal + 1));
        const auto comment = value.find('#');
        if (comment != std::string::npos) value = trim(value.substr(0, comment));
        if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') ||
                                  (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.size() - 2);
        }
        // The device prefix (usually cdrom0:) is not part of the ISO path.
        const auto colon = value.find(':');
        if (colon != std::string::npos) value = value.substr(colon + 1);
        value = trim(value);
        std::replace(value.begin(), value.end(), '\\', '/');
        while (!value.empty() && value.front() == '/') value.erase(value.begin());
        if (value.empty()) continue;
        if (key == "BOOT2") boot2 = value;
        else if (boot.empty()) boot = value;
    }
    executable_path = !boot2.empty() ? boot2 : boot;
    if (executable_path.empty()) {
        if (error) *error = "SYSTEM.CNF contains no usable BOOT2 or BOOT executable path";
        return false;
    }
    if (error) error->clear();
    return true;
}
} // namespace retroshelf
