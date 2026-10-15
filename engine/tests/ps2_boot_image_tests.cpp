#include "retroshelf/ps2_boot_image.hpp"
#include "retroshelf/elf_ram_loader.hpp"
#include "retroshelf/cpu.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr std::size_t sector = 2048;

void put16(std::vector<std::uint8_t>& b, std::size_t p, std::uint16_t v) {
    b[p] = static_cast<std::uint8_t>(v);
    b[p + 1] = static_cast<std::uint8_t>(v >> 8U);
}
void put32(std::vector<std::uint8_t>& b, std::size_t p, std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i)
        b[p + i] = static_cast<std::uint8_t>(v >> (8U * i));
}
std::size_t record(std::vector<std::uint8_t>& b, std::size_t p,
                   std::uint32_t extent, std::uint32_t size,
                   std::uint8_t flags, const std::string& name) {
    const auto n = name.size();
    const auto len = 33 + n + ((n % 2) == 0 ? 1 : 0);
    b[p] = static_cast<std::uint8_t>(len);
    b[p + 1] = 0;
    put32(b, p + 2, extent);
    put32(b, p + 10, size);
    b[p + 25] = flags;
    put16(b, p + 28, 1);
    b[p + 32] = static_cast<std::uint8_t>(n);
    for (std::size_t i = 0; i < n; ++i)
        b[p + 33 + i] = static_cast<std::uint8_t>(name[i]);
    return len;
}
std::vector<std::uint8_t> valid_elf() {
    // Minimal little-endian ELF32 MIPS executable with one PT_LOAD segment.
    std::vector<std::uint8_t> b(92, 0);
    b[0] = 0x7f; b[1] = 'E'; b[2] = 'L'; b[3] = 'F';
    b[4] = 1; b[5] = 1; b[6] = 1;
    put16(b, 16, 2); put16(b, 18, 8); put32(b, 20, 1);
    put32(b, 24, 0x00100000); put32(b, 28, 52);
    put16(b, 40, 52); put16(b, 42, 32); put16(b, 44, 1);
    put32(b, 52, 1); put32(b, 56, 84); put32(b, 60, 0x00100000);
    put32(b, 68, 8); put32(b, 72, 12); put32(b, 76, 5);
    b[84] = 0x07; b[85] = 0x00; b[86] = 0x08; b[87] = 0x24;\n    b[88] = 0x05; b[89] = 0x00; b[90] = 0x09; b[91] = 0x24;
    return b;
}
std::filesystem::path make_disc() {
    constexpr std::size_t sectors = 24;
    std::vector<std::uint8_t> image(sectors * sector, 0);
    auto* pvd = image.data() + 16 * sector;
    pvd[0] = 1; pvd[1] = 'C'; pvd[2] = 'D'; pvd[3] = '0';
    pvd[4] = '0'; pvd[5] = '1'; pvd[6] = 1;
    std::vector<std::uint8_t> descriptor(pvd, pvd + sector);
    record(descriptor, 156, 20, sector, 2, std::string(1, '\0'));
    std::copy(descriptor.begin() + 156, descriptor.begin() + 190,
              image.begin() + 16 * sector + 156);

    const std::string cnf = "BOOT2 = cdrom0:\\SLUS_123.45;1\n";
    const auto elf = valid_elf();
    std::vector<std::uint8_t> dir(sector, 0);
    std::size_t offset = 0;
    offset += record(dir, offset, 20, sector, 2, std::string(1, '\0'));
    offset += record(dir, offset, 20, sector, 2, std::string(1, '\1'));
    offset += record(dir, offset, 21, static_cast<std::uint32_t>(cnf.size()),
                     0, "SYSTEM.CNF;1");
    record(dir, offset, 22, static_cast<std::uint32_t>(elf.size()),
           0, "SLUS_123.45;1");
    std::copy(dir.begin(), dir.end(), image.begin() + 20 * sector);
    std::copy(cnf.begin(), cnf.end(), image.begin() + 21 * sector);
    std::copy(elf.begin(), elf.end(), image.begin() + 22 * sector);

    const auto path = std::filesystem::temp_directory_path() /
                      "retroshelf_ps2_boot_image_integration.iso";
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(image.data()),
               static_cast<std::streamsize>(image.size()));
    assert(file.good());
    return path;
}
} // namespace

int main() {
    const auto disc_path = make_disc();
    retroshelf::Iso9660Disc disc;
    retroshelf::Elf32MipsImage image;
    std::string executable_path;
    std::string error;
    assert(disc.open(disc_path, &error));
    assert(retroshelf::load_ps2_boot_image(disc, executable_path, image, &error));
    assert(executable_path == "SLUS_123.45;1");
    assert(image.entry_point == 0x00100000);
    assert(image.load_segments.size() == 1);
    assert(image.load_segments[0].virtual_address == 0x00100000);
    assert(image.load_segments[0].memory_size == 12);
    assert(image.load_segments[0].file_bytes.size() == 8);
    assert(image.load_segments[0].file_bytes[0] == 0x07);
    assert(image.load_segments[0].file_bytes[7] == 0x24);
    std::vector<std::uint8_t> ram(2 * 1024 * 1024, 0xA5);
    std::uint32_t entry = 0;
    assert(retroshelf::load_elf32_into_ram(image, 0, ram, entry, &error));
    assert(entry == 0x00100000);
    assert(ram[entry] == 0x07 && ram[entry + 1] == 0x00);
    assert(ram[entry + 8] == 0 && ram[entry + 11] == 0);

    retroshelf::Cpu cpu;
    cpu.set_pc(entry);
    assert(cpu.step_from_ram(ram, 0, &error));
    assert(cpu.reg(8) == 7);
    assert(cpu.pc() == entry + 4);
    assert(cpu.step_from_ram(ram, 0, &error));
    assert(cpu.reg(9) == 5);
    assert(cpu.pc() == entry + 8);
    std::filesystem::remove(disc_path);

    retroshelf::Iso9660Disc unopened;
    image.entry_point = 0x1234;
    executable_path = "stale-path";
    assert(!retroshelf::load_ps2_boot_image(unopened, executable_path, image, &error));
    assert(executable_path.empty());
    assert(image.load_segments.empty());
    assert(image.entry_point == 0);
    assert(!error.empty());
    std::cout << "PS2 disc-to-ELF integration tests passed\n";
}
