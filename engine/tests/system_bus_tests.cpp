#include "retroshelf/system_bus.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
class TestDevice final : public retroshelf::MmioDevice {
public:
 bool read(std::uint32_t offset,std::size_t size,std::vector<std::uint8_t>&data,std::string*error)const override{
  if(offset+size>registers.size()){if(error)*error="device read out of range";return false;}
  data.assign(registers.begin()+offset,registers.begin()+offset+size);return true;
 }
 bool write(std::uint32_t offset,const std::vector<std::uint8_t>&data,std::string*error)override{
  if(offset+data.size()>registers.size()){if(error)*error="device write out of range";return false;}
  std::copy(data.begin(),data.end(),registers.begin()+offset);return true;
 }
 std::vector<std::uint8_t> registers=std::vector<std::uint8_t>(16,0);
};
int main(){
 int failures=0;const auto check=[&](bool ok,const char*label){if(!ok){std::cerr<<"FAIL: "<<label<<"\n";++failures;}else std::cout<<"PASS: "<<label<<"\n";};
 std::vector<std::uint8_t>ram(32,0);retroshelf::SystemBus bus(ram);std::string error;
 auto device=std::make_shared<TestDevice>();
 check(bus.ram_size()==32,"bus reports its mapped RAM size");
 check(bus.map_mmio(0x1000,16,device,&error)&&error.empty(),"registers an MMIO device range");
 check(!bus.map_mmio(0x1008,8,std::make_shared<TestDevice>(),&error),"rejects overlapping MMIO mappings");
 check(!bus.map_mmio(0xFFFFFFF0U,32,std::make_shared<TestDevice>(),&error),"rejects address-space wrapping mappings");
 check(!bus.map_mmio(0x2000,0,device,&error),"rejects zero-sized mappings");
 check(bus.write(0x1004,{0x11,0x22,0x33,0x44},&error),"routes a 32-bit MMIO write");
 std::vector<std::uint8_t>data;
 check(bus.read(0x1004,4,data,&error)&&data==std::vector<std::uint8_t>({0x11,0x22,0x33,0x44}),"routes an MMIO read to the same device");
 check(!bus.read(0x1002,4,data,&error)&&data.empty(),"rejects unaligned MMIO reads and clears output");
 check(!bus.write(0x1001,{0xAA,0xBB},&error),"rejects unaligned MMIO writes");
 check(!bus.read(0x1000,3,data,&error)&&data.empty(),"rejects unsupported MMIO access widths");
 check(!bus.read(0x100C,8,data,&error)&&data.empty(),"rejects reads crossing a device mapping boundary");
 check(bus.write(16,std::vector<std::uint8_t>(16,0xA5),&error)&&error.empty(),"RAM write ending at its boundary succeeds");
 check(bus.read(16,16,data,&error)&&data==std::vector<std::uint8_t>(16,0xA5),"RAM reads preserve byte order");
 check(bus.read(32,0,data,&error)&&data.empty(),"zero-byte access at end of RAM succeeds");
 check(!bus.read(32,1,data,&error)&&data.empty(),"read beyond RAM fails and clears output");
 const auto before=ram;
 check(!bus.write(31,{0x11,0x22},&error)&&ram==before,"out-of-range RAM write is atomic");
 check(!bus.read(UINT32_MAX,2,data,&error),"large address cannot overflow RAM range validation");
 return failures==0?0:1;
}
