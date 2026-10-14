#include "retroshelf/gs_command_processor.hpp"

#include <algorithm>

namespace retroshelf::gs {
namespace {
constexpr std::uint8_t RegPrim = 0x00;
constexpr std::uint8_t RegRgbaq = 0x01;
constexpr std::uint8_t RegXyz2 = 0x05;
constexpr std::uint8_t RegXyoffset1 = 0x18;
constexpr std::uint8_t RegXyoffset2 = 0x19;
constexpr std::uint8_t GifAdRegister = 0xE;
}

GsCommandProcessor::GsCommandProcessor(std::uint32_t width, std::uint32_t height)
    : renderer_(width, height) { reset(); }

void GsCommandProcessor::reset() {
    for (auto& value : registers_) value = 0;
    vertex_count_ = 0;
    triangles_drawn_ = 0;
    rgba_ = 0xFFFFFFFFU;
    renderer_.clear(0x000000FFU);
}

std::uint64_t GsCommandProcessor::register_value(std::uint8_t address) const {
    return registers_[address & 0x7FU];
}

bool GsCommandProcessor::execute_gif(const std::uint64_t* words,
                                     std::size_t qword_count,
                                     std::string* error) {
    std::vector<GifPacketView> packets;
    std::size_t consumed = 0;
    if (!parse_gif_stream(words, qword_count, packets, consumed)) {
        if (error) *error = "Malformed or incomplete GIF stream (missing EOP or payload)";
        return false;
    }

    // This first execution path deliberately supports only PACKED A+D writes.
    // Unsupported packet formats fail explicitly instead of drawing guessed output.
    for (const auto& packet : packets) {
        if (packet.tag.format != 0) {
            if (error) *error = "GIF execution currently supports PACKED A+D packets only";
            return false;
        }
        for (std::size_t i = 0; i < packet.payload_qwords; ++i) {
            const std::size_t offset = (packet.payload_qword_offset + i) * 2U;
            const std::uint8_t reg = packet.tag.registers[i % packet.tag.nreg];
            if (reg != GifAdRegister) {
                if (error) *error = "GIF PACKED non-A+D register descriptors are not implemented";
                return false;
            }
            const auto value = words[offset];
            const auto address = static_cast<std::uint8_t>(words[offset + 1U] & 0x7FU);
            if (!write_register(address, value)) {
                if (error) *error = "Unsupported or invalid GS register write";
                return false;
            }
        }
    }
    if (error) error->clear();
    return true;
}

bool GsCommandProcessor::write_register(std::uint8_t address, std::uint64_t value) {
    address &= 0x7FU;
    registers_[address] = value;
    switch (address) {
        case RegPrim:
            // A change in primitive topology must not join vertices from two modes.
            vertex_count_ = 0;
            return true;
        case RegRgbaq:
            rgba_ = static_cast<std::uint32_t>(value >> 32U);
            return true;
        case RegXyz2:
            submit_vertex(value);
            return true;
        case RegXyoffset1:
        case RegXyoffset2:
            return true;
        default:
            // Keep ordinary state writes observable, but don't pretend unsupported
            // texture/framebuffer/depth registers are rendered.
            return address <= 0x7FU;
    }
}

void GsCommandProcessor::submit_vertex(std::uint64_t value) {
    const auto prim = static_cast<std::uint32_t>(registers_[RegPrim] & 0x7FFU);
    const auto primitive_type = prim & 0x7U;
    if (primitive_type != 3U) { // GS PRIM=3: triangle list
        vertex_count_ = 0;
        return;
    }
    const auto x_offset = static_cast<std::uint16_t>(registers_[RegXyoffset1] & 0xFFFFU);
    const auto y_offset = static_cast<std::uint16_t>((registers_[RegXyoffset1] >> 32U) & 0xFFFFU);
    const auto x_raw = static_cast<std::uint16_t>(value & 0xFFFFU);
    const auto y_raw = static_cast<std::uint16_t>((value >> 16U) & 0xFFFFU);
    // GS XY coordinates and XYOFFSET are signed 12.4 fixed-point quantities.
    const auto sx = static_cast<std::int16_t>(x_raw - x_offset);
    const auto sy = static_cast<std::int16_t>(y_raw - y_offset);
    vertices_[vertex_count_++] = {
        static_cast<float>(sx) / 16.0F,
        static_cast<float>(sy) / 16.0F,
        rgba_
    };
    if (vertex_count_ == 3) {
        if (renderer_.draw_triangle(vertices_[0], vertices_[1], vertices_[2]))
            ++triangles_drawn_;
        vertex_count_ = 0;
    }
}

} // namespace retroshelf::gs
