#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf {

// Converts PACKED and REGLIST GIF payloads into ordered GS register writes.
// IMAGE and IMAGE2 payloads are not decoded by this bring-up implementation.
class GsRegisterDecoder {
public:
    struct Write { std::uint8_t address; std::uint64_t value; };
    bool decode_gif(const std::vector<std::uint8_t>& packet);
    // Backward-compatible entry point; now accepts PACKED and REGLIST tags.
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
