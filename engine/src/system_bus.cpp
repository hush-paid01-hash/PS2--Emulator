#include "retroshelf/system_bus.hpp"
#include <algorithm>
namespace retroshelf {
SystemBus::SystemBus(std::vector<std::uint8_t>& ee_ram) : ee_ram_(ee_ram) {}
bool SystemBus::map_mmio(std::uint32_t base, std::uint32_t size, std::shared_ptr<MmioDevice> device, std::string* error) {
 if (!size || !device) { if(error)*error="MMIO mapping requires a non-zero size and a device"; return false; }
 const std::uint64_t end=static_cast<std::uint64_t>(base)+size;
 if(end>(std::uint64_t{1}<<32)){if(error)*error="MMIO mapping wraps the 32-bit address space";return false;}
 for(const auto& m:mmio_mappings_) if(static_cast<std::uint64_t>(base)<m.end && static_cast<std::uint64_t>(m.base)<end){if(error)*error="MMIO mapping overlaps an existing device";return false;}
 mmio_mappings_.push_back({base,end,std::move(device)});
 std::sort(mmio_mappings_.begin(),mmio_mappings_.end(),[](const MmioMapping&a,const MmioMapping&b){return a.base<b.base;});
 if(error)error->clear(); return true;
}
bool SystemBus::valid_range(std::uint32_t address,std::size_t size)const{const auto start=static_cast<std::size_t>(address);return start<=ee_ram_.size()&&size<=ee_ram_.size()-start;}
const SystemBus::MmioMapping* SystemBus::find_mmio(std::uint32_t address,std::size_t size)const{
 const std::uint64_t start=address,end=start+size;
 for(const auto&m:mmio_mappings_)if(start>=m.base&&start<m.end)return end<=m.end?&m:nullptr;
 return nullptr;
}
bool SystemBus::valid_mmio_access(std::uint32_t a,std::size_t s,std::string*e)const{
 if(s!=1&&s!=2&&s!=4&&s!=8){if(e)*e="MMIO access width must be 1, 2, 4, or 8 bytes";return false;}
 if(a%s){if(e)*e="MMIO access is not naturally aligned";return false;}return true;
}
bool SystemBus::read(std::uint32_t a,std::size_t s,std::vector<std::uint8_t>&d,std::string*e)const{
 if(const auto*m=find_mmio(a,s)){
  if(!valid_mmio_access(a,s,e)){d.clear();return false;}
  if(!m->device->read(a-m->base,s,d,e)||d.size()!=s){if(e&&e->empty())*e="MMIO device returned an invalid read width";d.clear();return false;}
  if(e)e->clear();return true;
 }
 for(const auto&m:mmio_mappings_)if(a>=m.base&&static_cast<std::uint64_t>(a)<m.end){d.clear();if(e)*e="read crosses the end of an MMIO mapping";return false;}
 if(!valid_range(a,s)){if(e)*e="read is outside EE RAM";d.clear();return false;}
 const auto start=static_cast<std::size_t>(a);d.assign(ee_ram_.begin()+start,ee_ram_.begin()+start+s);if(e)e->clear();return true;
}
bool SystemBus::write(std::uint32_t a,const std::vector<std::uint8_t>&d,std::string*e){
 if(const auto*m=find_mmio(a,d.size())){
  if(!valid_mmio_access(a,d.size(),e))return false;
  if(!m->device->write(a-m->base,d,e))return false;if(e)e->clear();return true;
 }
 for(const auto&m:mmio_mappings_)if(a>=m.base&&static_cast<std::uint64_t>(a)<m.end){if(e)*e="write crosses the end of an MMIO mapping";return false;}
 if(!valid_range(a,d.size())){if(e)*e="write is outside EE RAM";return false;}
 const auto start=static_cast<std::size_t>(a);std::copy(d.begin(),d.end(),ee_ram_.begin()+start);if(e)e->clear();return true;
}
std::size_t SystemBus::ram_size()const{return ee_ram_.size();}
} // namespace retroshelf
