#include "retroshelf/disc_image.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr std::size_t Sector = 2048;
void put_le16(std::uint8_t* p, std::uint16_t value) {
    p[0] = static_cast<std::uint8_t>(value); p[1] = static_cast<std::uint8_t>(value >> 8U);
}
void put_le32(std::uint8_t* p, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) p[i] = static_cast<std::uint8_t>(value >> (8U * i));
}
void put_be32(std::uint8_t* p, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) p[i] = static_cast<std::uint8_t>(value >> (8U * (3U - i)));
}
std::size_t add_record(std::uint8_t* target, std::uint32_t lba, std::uint32_t size,
                       std::uint8_t flags, const std::string& name) {
    const auto name_length = name.size();
    const auto length = static_cast<std::uint8_t>(33 + name_length + (name_length % 2 == 0 ? 1 : 0));
    target[0] = length; target[1] = 0;
    put_le32(target + 2, lba); put_be32(target + 6, lba);
    put_le32(target + 10, size); put_be32(target + 14, size);
    target[18] = 126; target[19] = 1; target[20] = 1; target[21] = 0;
    target[22] = 0; target[23] = 0; target[24] = 0;
    target[25] = flags; put_le16(target + 28, 1); target[30] = 0; target[31] = 0;
    target[32] = static_cast<std::uint8_t>(name_length);
    std::copy(name.begin(), name.end(), target + 33);
    return length;
}
}

int main() {
    int failures = 0;
    const auto check = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << "FAIL: " << label << '\n'; ++failures; }
        else std::cout << "PASS: " << label << '\n';
    };
    const std::string path = "retroshelf-test-disc.iso";
    constexpr std::uint32_t sectors = 24;
    std::vector<std::uint8_t> image(sectors * Sector, 0);
    auto* pvd = image.data() + 16 * Sector;
    pvd[0] = 1; pvd[1] = 'C'; pvd[2] = 'D'; pvd[3] = '0'; pvd[4] = '0'; pvd[5] = '1'; pvd[6] = 1;
    const std::string title = "RETROSHELF TEST";
    std::copy(title.begin(), title.end(), pvd + 40);
    put_le32(pvd + 80, sectors); put_be32(pvd + 84, sectors);
    put_le16(pvd + 128, 2048); pvd[130] = 8; pvd[131] = 0;

    // Root directory lives at sector 18. SYSTEM.CNF is sector 19; the
    // referenced boot executable is sector 20. These are synthetic test data.
    auto* root_record = pvd + 156;
    add_record(root_record, 18, Sector, 2, std::string(1, '\0'));
    auto* root = image.data() + 18 * Sector;
    std::size_t used = 0;
    used += add_record(root + used, 18, Sector, 2, std::string(1, '\0'));
    used += add_record(root + used, 18, Sector, 2, std::string(1, '\1'));
    used += add_record(root + used, 19, 34, 0, "SYSTEM.CNF;1");
    used += add_record(root + used, 20, 8, 0, "SLUS_123.45;1");
    const std::string config = "BOOT2 = cdrom0:\\SLUS_123.45;1\nVER = 1.00\n";
    std::copy(config.begin(), config.end(), image.begin() + 19 * Sector);
    const std::string elf_marker = "PS2ELF!!";
    std::copy(elf_marker.begin(), elf_marker.end(), image.begin() + 20 * Sector);
    {
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(image.data()), static_cast<std::streamsize>(image.size()));
    }
    const auto valid = retroshelf::media::inspect_iso9660(path);
    check(valid.valid, "recognizes valid ISO-9660 primary volume descriptor");
    check(valid.volume_sectors == sectors && valid.logical_block_size == Sector, "reports volume geometry");
    check(valid.volume_id == title, "reads volume identifier");
    std::vector<std::uint8_t> sector_data;
    std::string sector_error;
    check(retroshelf::media::read_iso_sector(path, 16, sector_data, &sector_error), "reads in-range ISO sector");
    check(sector_data.size() == Sector && sector_data[0] == 1, "returns exact 2048-byte sector");
    check(!retroshelf::media::read_iso_sector(path, sectors, sector_data, &sector_error), "rejects out-of-range sector");
    check(sector_error.find("outside") != std::string::npos, "out-of-range sector has diagnostic");

    std::vector<retroshelf::media::IsoDirectoryEntry> entries;
    check(retroshelf::media::read_iso_directory(path, entries, &sector_error), "parses ISO root directory");
    check(entries.size() == 2, "filters dot entries from ISO directory");
    check(entries.size() >= 2 && entries[0].name == "SYSTEM.CNF" && entries[1].name == "SLUS_123.45",
          "normalizes ISO file names by removing version suffix");
    std::vector<std::uint8_t> file_data;
    check(retroshelf::media::read_iso_file(path, 19, 34, file_data, &sector_error), "reads file extent");
    check(std::string(file_data.begin(), file_data.end()).find("BOOT2") != std::string::npos,
          "file reader returns SYSTEM.CNF contents");
    check(!retroshelf::media::read_iso_file(path, sectors - 1, Sector * 2, file_data, &sector_error),
          "rejects file extent extending beyond image");

    const auto boot = retroshelf::media::find_ps2_boot_executable(path);
    check(boot.found, "disc parser discovers boot executable referenced by SYSTEM.CNF");
    check(boot.boot_path.find("SLUS_123.45") != std::string::npos, "preserves boot path from SYSTEM.CNF");
    check(boot.executable_lba == 20 && boot.executable_size == 8, "returns boot executable extent");

    image[16 * Sector + 1] = 'X';
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file.write(reinterpret_cast<const char*>(image.data()), static_cast<std::streamsize>(image.size()));
    }
    check(!retroshelf::media::inspect_iso9660(path).valid, "rejects invalid descriptor signature");
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc); file << "not an ISO";
    }
    check(!retroshelf::media::inspect_iso9660(path).valid, "rejects truncated image");
    std::remove(path.c_str());
    check(!retroshelf::media::inspect_iso9660(path).valid, "reports missing image file");
    return failures == 0 ? 0 : 1;
}
