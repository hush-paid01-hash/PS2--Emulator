#include "retroshelf/ps2_boot_config.hpp"

#include <cassert>
#include <iostream>
#include <string>

int main() {
    std::string path, error;
    const std::string config =
        "BOOT = cdrom:\\SLUS_111.11;1\r\n"
        "VER = 1.00\r\n"
        "BOOT2 = cdrom0:\\SLUS_222.22;1\r\n"
        "VMODE = NTSC\r\n";
    assert(retroshelf::parse_system_cnf(config, path, &error));
    assert(path == "SLUS_222.22;1");
    assert(error.empty());

    assert(retroshelf::parse_system_cnf("BOOT = cdrom:\\SLUS_333.33;1\n", path, &error));
    assert(path == "SLUS_333.33;1");

    assert(!retroshelf::parse_system_cnf("VER = 1.00\nVMODE = NTSC\n", path, &error));
    assert(path.empty());
    assert(!error.empty());
    std::cout << "PS2 SYSTEM.CNF parser tests passed\n";
}
