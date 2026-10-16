#include "retroshelf/engine_c_api.h"

#include "retroshelf/iso9660_disc.hpp"
#include "retroshelf/ps2_boot_image.hpp"
#include "retroshelf/bios_image_validator.hpp"

#include <filesystem>
#include <string>

extern "C" uint32_t retroshelf_engine_abi_version(void) {
    return 1U;
}

extern "C" const char *retroshelf_engine_status_message(void) {
    return "Development foundation only; BIOS boot and game rendering are not implemented.";
}

extern "C" int32_t retroshelf_engine_can_boot_games(void) {
    return 0;
}

extern "C" int32_t retroshelf_engine_validate_disc_boot_image(const char *path) {
    if (path == nullptr || path[0] == '\0') return 0;

    retroshelf::Iso9660Disc disc;
    std::string error;
    if (!disc.open(std::filesystem::path(path), &error)) return 0;

    std::string executable_path;
    retroshelf::Elf32MipsImage image;
    return retroshelf::load_ps2_boot_image(disc, executable_path, image, &error) ? 1 : 0;
}

extern "C" int32_t retroshelf_engine_inspect_bios_image(const char *path) {
    if (path == nullptr || path[0] == '\0') return 0;

    retroshelf::BiosImageInfo info;
    std::string error;
    if (!retroshelf::BiosImageValidator::inspect(std::filesystem::path(path), info, &error))
        return 0;
    return info.plausible_size ? 1 : 2;
}
