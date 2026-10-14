#include "retroshelf/gs_gif_processor.hpp"

#include <array>
#include <cmath>

namespace retroshelf::gs {
namespace {
struct State {
    std::uint64_t prim = 0;
    std::uint32_t rgba = 0xFFFFFFFFU;
    std::array<ScreenVertex, 3> vertices{};
    std::size_t vertex_count = 0;
    GifProcessStats stats{};
};
bool fail(std::string* error, const char* message) {
    if (error) *error = message;
    return false;
}
bool write_register(std::uint8_t reg, std::uint64_t value,
                    SoftwareRenderer& renderer, State& state, std::string* error) {
    ++state.stats.register_writes;
    switch (reg) {
        case 0x0: // PRIM
            state.prim = value;
            state.vertex_count = 0;
            return true;
        case 0x1: // RGBAQ (Q is ignored for flat screen-space rasterization)
            state.rgba = ((static_cast<std::uint32_t>(value) & 0xFFU) << 24U) |
                         ((static_cast<std::uint32_t>(value >> 8U) & 0xFFU) << 16U) |
                         ((static_cast<std::uint32_t>(value >> 16U) & 0xFFU) << 8U) |
                         (static_cast<std::uint32_t>(value >> 24U) & 0xFFU);
            return true;
        case 0x5: { // XYZ2; X/Y are 12.4 fixed-point screen coordinates.
            const auto x = static_cast<float>(value & 0xFFFFU) / 16.0F;
            const auto y = static_cast<float>((value >> 16U) & 0xFFFFU) / 16.0F;
            const auto primitive_type = static_cast<unsigned>(state.prim & 0x7U);
            if (primitive_type != 3U) {
                return fail(error, "GIF processor currently supports triangle-list PRIM only");
            }
            state.vertices[state.vertex_count++] = {x, y, state.rgba};
            if (state.vertex_count == 3) {
                if (!renderer.draw_triangle(state.vertices[0], state.vertices[1], state.vertices[2])) {
                    return fail(error, "GS software renderer rejected a triangle");
                }
                ++state.stats.triangles_drawn;
                state.vertex_count = 0;
            }
            return true;
        }
        default:
            return fail(error, "Unsupported packed GS register; refusing to silently skip command");
    }
}
} // namespace

bool process_gif_stream(const std::uint64_t* words, std::size_t qword_count,
                        SoftwareRenderer& renderer, GifProcessStats* stats,
                        std::string* error) {
    if (stats) *stats = {};
    if (error) error->clear();
    std::vector<GifPacketView> packets;
    std::size_t consumed = 0;
    if (!parse_gif_stream(words, qword_count, packets, consumed)) {
        return fail(error, "Malformed or truncated GIF stream (including missing EOP)");
    }
    State state{};
    for (const auto& packet : packets) {
        ++state.stats.packets;
        if (packet.tag.format != 0) {
            return fail(error, "Only PACKED GIF mode is currently executable");
        }
        for (std::size_t loop = 0; loop < packet.tag.nloop; ++loop) {
            for (std::size_t r = 0; r < packet.tag.nreg; ++r) {
                const auto qword = packet.payload_qword_offset + loop * packet.tag.nreg + r;
                if (!write_register(packet.tag.registers[r], words[qword * 2],
                                    renderer, state, error)) return false;
            }
        }
    }
    if (state.vertex_count != 0) {
        return fail(error, "GIF stream ended with an incomplete triangle");
    }
    if (stats) *stats = state.stats;
    return true;
}

} // namespace retroshelf::gs
