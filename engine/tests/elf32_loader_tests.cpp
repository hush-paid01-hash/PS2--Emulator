#include "retroshelf/elf32_loader.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

namespace {
void put16(std::vector<std::uint8_t>& b, std::size_t p, std::uint16_t v) {
    b[p] = static_cast<std::uint8_t>(v);
    b[p + 1] = static_cast<std::uint8_t>(v >> 8U);
}
void put32(std::vector<std::uint8_t>& b, std::size_t p, std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i)
        b[p + i] = static_cast<std::uint8_t>(v >> (i * 8U));
}
std::vector<std::uint8_t> fixture() {
    std::vector<std::uint8_t> b(0x104, 0);
    b[0]=0x7f; b[1]='E'; b[2]='L'; b[3]='F';
    b[4]=1; b[5]=1; b[6]=1;
    put16(b,16,2); put16(b,18,8); put32(b,20,1);
    put32(b,24,0x100); put32(b,28,52);
    put16(b,40,52); put16(b,42,32); put16(b,44,1);
    put32(b,52,1); put32(b,56,0x100); put32(b,60,0x100);
    put32(b,64,0x100); put32(b,68,4); put32(b,72,8);
    b[0x100]=0x11; b[0x101]=0x22; b[0x102]=0x33; b[0x103]=0x44;
    return b;
}
}
int main() {
    int failures=0;
    const auto check=[&](bool ok,const char* name){
        if(!ok){std::cerr<<"FAIL: "<<name<<'\n';++failures;}
        else std::cout<<"PASS: "<<name<<'\n';
    };
    std::vector<std::uint8_t> ram(1024,0xAA);
    auto image=fixture();
    auto result=retroshelf::Elf32Loader::load_mips_executable(image,ram);
    check(result.ok && result.entry_point==0x100 && result.segments_loaded==1,
          "loads MIPS ELF32 and returns entry point");
    check(ram[0x100]==0x11 && ram[0x101]==0x22 &&
          ram[0x102]==0x33 && ram[0x103]==0x44,
          "copies segment bytes into EE RAM");
    check(ram[0x104]==0 && ram[0x107]==0,
          "zero-fills segment memory beyond file data");

    auto bad=fixture(); put32(bad,68,0x1000); put32(bad,72,0x1000);
    const auto before=ram;
    result=retroshelf::Elf32Loader::load_mips_executable(bad,ram);
    check(!result.ok && ram==before,
          "rejects out-of-range segment without partial RAM mutation");
    result=retroshelf::Elf32Loader::load_mips_executable({1,2,3},ram);
    check(!result.ok,"rejects truncated/non-ELF input");
    auto wrong=fixture(); put16(wrong,18,3);
    check(!retroshelf::Elf32Loader::load_mips_executable(wrong,ram).ok,
          "rejects non-MIPS ELF machine type");
    return failures==0?0:1;
}
