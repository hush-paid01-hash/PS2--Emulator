#include "retroshelf/disc_image.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int main() {
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << '\n'; ++failures; }
        else std::cout << "PASS: " << label << '\n';
    };
    const std::string path = "retroshelf-test-disc.iso";
    constexpr std::size_t sector = 2048;
    std::vector<std::uint8_t> image(20 * sector, 0);
    auto* pvd = image.data() + 16 * sector;
    pvd[0] = 1; pvd[1] = 'C'; pvd[2] = 'D'; pvd[3] = '0'; pvd[4] = '0'; pvd[5] = '1'; pvd[6] = 1;
    const std::string title = "RETROSHELF TEST";
    for (std::size_t i = 0; i < title.size(); ++i) pvd[40 + i] = static_cast<std::uint8_t>(title[i]);
    const std::uint32_t sectors = 20;
    for (unsigned i = 0; i < 4; ++i) {
        pvd[80 + i] = static_cast<std::uint8_t>((sectors >> (8U * i)) & 0xFFU);
        pvd[84 + i] = static_cast<std::uint8_t>((sectors >> (8U * (3U - i))) & 0xFFU);
    }
    pvd[128] = 0; pvd[129] = 8; pvd[130] = 8; pvd[131] = 0;
    {
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(image.data()), static_cast<std::streamsize>(image.size()));
    }
    const auto valid = retroshelf::media::inspect_iso9660(path);
    check(valid.valid, "recognizes valid ISO-9660 primary volume descriptor");
    check(valid.volume_sectors == 20 && valid.logical_block_size == 2048, "reports volume geometry");
    check(valid.volume_id == title, "reads volume identifier");

    image[16 * sector + 1] = 'X';
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file.write(reinterpret_cast<const char*>(image.data()), static_cast<std::streamsize>(image.size()));
    }
    check(!retroshelf::media::inspect_iso9660(path).valid, "rejects invalid descriptor signature");
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file << "not an ISO";
    }
    check(!retroshelf::media::inspect_iso9660(path).valid, "rejects truncated image");
    std::remove(path.c_str());
    check(!retroshelf::media::inspect_iso9660(path).valid, "reports missing image file");
    return failures == 0 ? 0 : 1;
}
