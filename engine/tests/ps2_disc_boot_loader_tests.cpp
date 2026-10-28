#include "retroshelf/iso9660_disc.hpp"
#include "retroshelf/ps2_disc_boot_loader.hpp"

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
    b[p] = static_cast<std::uint8_t>(v); b[p+1] = static_cast<std::uint8_t>(v >> 8U);
}
void put32(std::vector<std::uint8_t>& b, std::size_t p, std::uint32_t v) {
    for (unsigned i=0;i<4;++i) b[p+i] = static_cast<std::uint8_t>(v >> (8U*i));
}
std::size_t record(std::vector<std::uint8_t>& b, std::size_t p, std::uint32_t extent,
                   std::uint32_t size, std::uint8_t flags, const std::string& name) {
    const auto n=name.size();
    const auto len=33+n+((n%2)==0?1:0);
    b[p]=static_cast<std::uint8_t>(len); b[p+1]=0;
    put32(b,p+2,extent); put32(b,p+10,size); b[p+25]=flags;
    put16(b,p+28,1); b[p+32]=static_cast<std::uint8_t>(n);
    for(std::size_t i=0;i<n;++i)b[p+33+i]=static_cast<std::uint8_t>(name[i]);
    return len;
}
}

int main() {
    constexpr std::size_t sectors=24;
    const auto path=std::filesystem::temp_directory_path()/"retroshelf_boot_loader_test.iso";
    std::vector<std::uint8_t> image(sectors*sector,0);
    auto* pvd=image.data()+16*sector;
    pvd[0]=1;pvd[1]='C';pvd[2]='D';pvd[3]='0';pvd[4]='0';pvd[5]='1';pvd[6]=1;
    std::vector<std::uint8_t> descriptor(pvd,pvd+sector);
    record(descriptor,156,20,sector,2,std::string(1,'\0'));
    std::copy(descriptor.begin()+156,descriptor.begin()+190,image.begin()+16*sector+156);

    const std::string cnf="BOOT2 = cdrom0:\\SLUS_123.45;1\nBOOT = cdrom0:\\FALLBACK.ELF;1\n";
    const std::vector<std::uint8_t> elf_stub={0x7f,'E','L','F',1,2,1,0};
    std::vector<std::uint8_t> dir(sector,0);
    std::size_t offset=0;
    offset+=record(dir,offset,20,sector,2,std::string(1,'\0'));
    offset+=record(dir,offset,20,sector,2,std::string(1,'\1'));
    offset+=record(dir,offset,21,static_cast<std::uint32_t>(cnf.size()),0,"SYSTEM.CNF;1");
    record(dir,offset,22,static_cast<std::uint32_t>(elf_stub.size()),0,"SLUS_123.45;1");
    std::copy(dir.begin(),dir.end(),image.begin()+20*sector);
    std::copy(cnf.begin(),cnf.end(),image.begin()+21*sector);
    std::copy(elf_stub.begin(),elf_stub.end(),image.begin()+22*sector);
    {std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char*>(image.data()),static_cast<std::streamsize>(image.size()));}

    retroshelf::Iso9660Disc disc;
    std::string error, executable_path;
    std::vector<std::uint8_t> executable;
    assert(disc.open(path,&error));
    assert(retroshelf::load_ps2_boot_executable(disc,executable_path,executable,&error));
    assert(executable_path=="SLUS_123.45;1");
    assert(executable==elf_stub);
    std::filesystem::remove(path);

    retroshelf::Iso9660Disc unopened;
    assert(!retroshelf::load_ps2_boot_executable(unopened,executable_path,executable,&error));
    assert(executable.empty());
    std::cout<<"PS2 disc boot loader tests passed\n";
}
