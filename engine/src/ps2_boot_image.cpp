#include "retroshelf/ps2_boot_image.hpp"

#include "retroshelf/ps2_disc_boot_loader.hpp"

#include <vector>

namespace retroshelf {

bool load_ps2_boot_image(const Iso9660Disc& disc,
                         std::string& executable_path,
                         Elf32MipsImage& image,
                         std::string* error) {
    image = {};
    std::vector<std::uint8_t> executable;
    if (!load_ps2_boot_executable(disc, executable_path, executable, error)) {
        executable_path.clear();
        return false;
    }
    if (!parse_elf32_mips(executable, image, error)) {
        executable_path.clear();
        return false;
    }
    if (error) error->clear();
    return true;
}

} // namespace retroshelf
