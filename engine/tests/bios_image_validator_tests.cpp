#include "retroshelf/bios_image_validator.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

int main() {
    retroshelf::BiosImageInfo info;
    std::string error;
    assert(!retroshelf::BiosImageValidator::inspect({}, info, &error));
    assert(!error.empty());
    assert(info.size_bytes == 0);

    const auto dir = std::filesystem::temp_directory_path() / "retroshelf_bios_validator_tests";
    std::filesystem::create_directories(dir);
    const auto tiny = dir / "tiny.bin";
    { std::ofstream out(tiny, std::ios::binary); out.put('\0'); }
    assert(retroshelf::BiosImageValidator::inspect(tiny, info, &error));
    assert(info.size_bytes == 1);
    assert(!info.plausible_size);

    const auto plausible = dir / "plausible.bin";
    { std::ofstream out(plausible, std::ios::binary); out.seekp(4 * 1024 * 1024 - 1); out.put('\0'); }
    assert(retroshelf::BiosImageValidator::inspect(plausible, info, &error));
    assert(info.size_bytes == 4ULL * 1024 * 1024);
    assert(info.plausible_size);
    assert(error.empty());

    assert(!retroshelf::BiosImageValidator::inspect(dir / "missing.bin", info, &error));
    std::filesystem::remove_all(dir);
}
