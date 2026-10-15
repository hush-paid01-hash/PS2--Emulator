#include "retroshelf/ps2_boot_image.hpp"

#include "retroshelf/ps2_disc_boot_loader.hpp"
#include "retroshelf/elf_ram_loader.hpp"

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

bool boot_ps2_disc_into_ee_memory(const Iso9660Disc& disc,
                                  Ps2EeMemoryMap& memory,
                                  std::string& executable_path,
                                  std::uint32_t& entry_point,
                                  std::string* error) {
    executable_path.clear();
    entry_point = 0;
    Elf32MipsImage image;
    if (!load_ps2_boot_image(disc, executable_path, image, error)) return false;
    if (!load_elf32_into_ee_memory(image, memory, entry_point, error)) {
        executable_path.clear();
        entry_point = 0;
        return false;
    }
    if (error) error->clear();
    return true;
}

} // namespace retroshelf
