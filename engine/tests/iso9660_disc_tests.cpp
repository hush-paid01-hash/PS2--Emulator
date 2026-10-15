#include "retroshelf/iso9660_disc.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
void put16(std::vector<std::uint8_t>& b, std::size_t p, std::uint16_t v) {
    b[p] = static_cast<std::uint8_t>(v); b[p+1] = static_cast<std::uint8_t>(v >> 8U);
}
void put32(std::vector<std::uint8_t>& b, std::size_t p, std::uint32_t v) {
    b[p] = static_cast<std::uint8_t>(v); b[p+1] = static_cast<std::uint8_t>(v >> 8U);
    b[p+2] = static_cast<std::uint8_t>(v >> 16U); b[p+3] = static_cast<std::uint8_t>(v >> 24U);
}
void record(std::vector<std::uint8_t>& b, std::size_t p, std::uint32_t extent,
            std::uint32_t size, std::uint8_t flags, const std::string& name) {
    const auto n = name.size();
    const auto length = 33 + n + ((n % 2) == 0 ? 1 : 0);
    b[p] = static_cast<std::uint8_t>(length); b[p+1] = 0;
    put32(b,p+2,extent); put32(b,p+10,size); b[p+25] = flags;
    put16(b,p+28,1); b[p+32] = static_cast<std::uint8_t>(n);
    for (std::size_t i=0;i<n;++i) b[p+33+i] = static_cast<std::uint8_t>(name[i]);
}
}
int main() {
    constexpr std::size_t sectors = 24, sector = 2048;
    std::vector<std::uint8_t> image(sectors * sector, 0);
    auto* pvd = image.data() + 16 * sector;
    pvd[0]=1; pvd[1]='C'; pvd[2]='D'; pvd[3]='0'; pvd[4]='0'; pvd[5]='1'; pvd[6]=1;
    std::vector<std::uint8_t> descriptor(pvd,pvd+sector);
    record(descriptor,156,20,sector,2,std::string(1,'\0'));
    std::copy(descriptor.begin()+156,descriptor.begin()+190,image.begin()+16*sector+156);
    auto* dir = image.data() + 20 * sector;
    std::vector<std::uint8_t> directory(sector,0);
    record(directory,0,20,sector,2,std::string(1,'\0'));
    record(directory,34,20,sector,2,std::string(1,'\1'));
    record(directory,68,21,5,0,"HELLO.TXT;1");
    std::copy(directory.begin(),directory.end(),dir);
    const std::string payload="hello";
    std::copy(payload.begin(),payload.end(),image.begin()+21*sector);
    const auto path=std::filesystem::temp_directory_path()/"retroshelf_iso_reader_test.iso";
    { std::ofstream f(path,std::ios::binary); f.write(reinterpret_cast<const char*>(image.data()),static_cast<std::streamsize>(image.size())); }
    retroshelf::Iso9660Disc disc; std::string error;
    assert(disc.open(path,&error) && error.empty());
    assert(disc.sector_count()==sectors);
    std::vector<std::uint8_t> bytes;
    assert(disc.read_file("/hello.txt",&bytes,&error));
    assert(std::string(bytes.begin(),bytes.end())==payload);
    assert(!disc.read_file("/missing.bin",&bytes,&error));
    std::filesystem::remove(path);
    std::cout << "ISO-9660 disc reader tests passed\n";
}
