#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf {

class SystemBus {
public:
    explicit SystemBus(std::vector<std::uint8_t>& ee_ram);

    bool read(std::uint32_t address, std::size_t size, std::vector<std::uint8_t>& data,
              std::string* error = nullptr) const;
    bool write(std::uint32_t address, const std::vector<std::uint8_t>& data,
               std::string* error = nullptr);
    std::size_t ram_size() const;

private:
    bool valid_range(std::uint32_t address, std::size_t size) const;

    std::vector<std::uint8_t>& ee_ram_;
};

} // namespace retroshelf
