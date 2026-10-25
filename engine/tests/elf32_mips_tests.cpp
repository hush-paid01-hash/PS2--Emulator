#include "retroshelf/elf32_mips.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {
void put16(std::vector<std::uint8_t>& b,std::size_t p,std::uint16_t v) {
    b[p]=static_cast<std::uint8_t>(v); b[p+1]=static_cast<std::uint8_t>(v>>8U);
}
void put32(std::vector<std::uint8_t>& b,std::size_t p,std::uint32_t v) {
    for(unsigned i=0;i<4;++i)b[p+i]=static_cast<std::uint8_t>(v>>(8U*i));
}
std::vector<std::uint8_t> valid_elf() {
    std::vector<std::uint8_t> b(88,0);
    b[0]=0x7f;b[1]='E';b[2]='L';b[3]='F';b[4]=1;b[5]=1;b[6]=1;
    put16(b,16,2);put16(b,18,8);put32(b,20,1);put32(b,24,0x00100000);
    put32(b,28,52);put16(b,40,52);put16(b,42,32);put16(b,44,1);
    put32(b,52,1);put32(b,56,84);put32(b,60,0x00100000);
    put32(b,68,4);put32(b,72,8);put32(b,76,5);
    b[84]=0x11;b[85]=0x22;b[86]=0x33;b[87]=0x44;
    return b;
}
}
int main() {
    retroshelf::Elf32MipsImage image;
    std::string error;
    auto bytes=valid_elf();
    assert(retroshelf::parse_elf32_mips(bytes,image,&error));
    assert(image.entry_point==0x00100000);
    assert(image.load_segments.size()==1);
    assert(image.load_segments[0].virtual_address==0x00100000);
    assert(image.load_segments[0].memory_size==8);
    assert(image.load_segments[0].file_bytes.size()==4);
    assert(image.load_segments[0].file_bytes[3]==0x44);
    assert(error.empty());

    bytes[0]=0;
    assert(!retroshelf::parse_elf32_mips(bytes,image,&error));
    assert(image.load_segments.empty());
    bytes=valid_elf();
    put32(bytes,56,0xfffffff0U);
    assert(!retroshelf::parse_elf32_mips(bytes,image,&error));

    bytes=valid_elf();
    put32(bytes,68,9); put32(bytes,72,8);
    assert(!retroshelf::parse_elf32_mips(bytes,image,&error));

    bytes=valid_elf();
    put32(bytes,24,0x00200000); // Entry point is outside the only PT_LOAD segment.
    assert(!retroshelf::parse_elf32_mips(bytes,image,&error));
    assert(image.load_segments.empty());
    assert(error=="ELF entry point is not inside an executable load segment");

    bytes=valid_elf();
    put32(bytes,76,4); // Segment is readable but not executable.
    assert(!retroshelf::parse_elf32_mips(bytes,image,&error));
    assert(image.load_segments.empty());
    assert(error=="ELF entry point is not inside an executable load segment");

    bytes=valid_elf();
    put32(bytes,60,0xFFFFFFFCU);
    put32(bytes,72,8); // Segment crosses the 32-bit address boundary.
    assert(!retroshelf::parse_elf32_mips(bytes,image,&error));
    assert(image.load_segments.empty());
    std::cout<<"ELF32 MIPS parser tests passed\n";
}
