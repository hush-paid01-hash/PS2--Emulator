#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf {

// Converts PACKED GIF payloads into ordered GS register writes.
// This is a decoder foundation, not yet a graphics renderer.
class GsRegisterDecoder {
public:
    struct Write { std::uint8_t address; std::uint64_t value; };
    bool decode_packed(const std::vector<std::uint8_t>& packet);
    const std::vector<Write>& writes() const;
    const std::string& error() const;
    void clear();
private:
    bool fail(const std::string& message);
    std::vector<Write> writes_;
    std::string error_;
};
} // namespace retroshelf
