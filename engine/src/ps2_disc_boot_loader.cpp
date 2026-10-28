#include "retroshelf/ps2_disc_boot_loader.hpp"

#include "retroshelf/ps2_boot_config.hpp"

namespace retroshelf {

bool load_ps2_boot_executable(const Iso9660Disc& disc,
                              std::string& executable_path,
                              std::vector<std::uint8_t>& executable,
                              std::string* error) {
    executable_path.clear();
    executable.clear();
    if (!disc.is_open()) {
        if (error) *error = "Disc image is not open";
        return false;
    }

    std::vector<std::uint8_t> config_bytes;
    if (!disc.read_file("/SYSTEM.CNF", config_bytes, error)) return false;
    const std::string config(config_bytes.begin(), config_bytes.end());
    if (!parse_system_cnf(config, executable_path, error)) return false;

    if (!disc.read_file(executable_path, executable, error)) {
        executable_path.clear();
        executable.clear();
        return false;
    }
    if (executable.empty()) {
        executable_path.clear();
        if (error) *error = "Boot executable file is empty";
        return false;
    }
    if (error) error->clear();
    return true;
}

} // namespace retroshelf
