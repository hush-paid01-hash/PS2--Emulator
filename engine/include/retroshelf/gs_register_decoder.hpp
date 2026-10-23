#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace retroshelf {

// Converts PACKED and REGLIST GIF payloads into ordered GS register writes,
// while preserving IMAGE and IMAGE2 payloads as raw transfer data.
class GsRegisterDecoder {
public:
    struct Write { std::uint8_t address; std::uint64_t value; };
    struct ImageTransfer {
        std::uint8_t format; // 2 = IMAGE, 3 = IMAGE2.
        std::vector<std::uint8_t> data; // Exact payload bytes, including padding.
    };
    bool decode_gif(const std::vector<std::uint8_t>& packet);
    bool decode_packed(const std::vector<std::uint8_t>& packet);
    const std::vector<Write>& writes() const;
    const std::vector<ImageTransfer>& image_transfers() const;
    const std::string& error() const;
    void clear();
private:
    bool fail(const std::string& message);
    std::vector<Write> writes_;
    std::vector<ImageTransfer> image_transfers_;
    std::string error_;
};
} // namespace retroshelf
