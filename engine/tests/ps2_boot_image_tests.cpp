#include "retroshelf/ps2_boot_image.hpp"

#include <cassert>
#include <iostream>
#include <string>

int main() {
    retroshelf::Iso9660Disc unopened;
    retroshelf::Elf32MipsImage image;
    image.entry_point = 0x1234;
    std::string path = "stale-path";
    std::string error;
    assert(!retroshelf::load_ps2_boot_image(unopened, path, image, &error));
    assert(path.empty());
    assert(image.load_segments.empty());
    assert(image.entry_point == 0);
    assert(!error.empty());
    std::cout << "PS2 boot image tests passed\n";
}
