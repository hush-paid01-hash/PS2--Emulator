#include "retroshelf/gs_command_processor.hpp"

namespace retroshelf {
namespace {
constexpr std::uint8_t GsPrim = 0x0;
constexpr std::uint8_t GsRgbaq = 0x1;
constexpr std::uint8_t GsXyz2 = 0x5;
constexpr std::uint64_t PrimitiveTypeMask = 0x7U;
constexpr std::uint64_t TriangleList = 3U;

GsFramebuffer::Pixel unpack_rgba(std::uint64_t value) {
    return {
        static_cast<std::uint8_t>(value & 0xFFU),
        static_cast<std::uint8_t>((value >> 8U) & 0xFFU),
        static_cast<std::uint8_t>((value >> 16U) & 0xFFU),
        static_cast<std::uint8_t>((value >> 24U) & 0xFFU)};
}
} // namespace

GsCommandProcessor::GsCommandProcessor(std::size_t width, std::size_t height)
    : framebuffer_(width, height) {}

bool GsCommandProcessor::submit_packed_packet(const std::vector<std::uint8_t>& packet) {
    error_.clear();
    if (!decoder_.decode_packed(packet)) {
        error_ = decoder_.error();
        return false;
    }

    // Decode fully before applying any writes so malformed packets do not
    // partially mutate GS state.
    for (const auto& write : decoder_.writes()) {
        if (!state_.write(write.address, write.value)) {
            error_ = "decoded GS register address is outside the register state";
            return false;
        }
        consume_render_register(write.address, write.value);
        ++writes_processed_;
    }
    ++packets_processed_;
    return true;
}

void GsCommandProcessor::consume_render_register(std::uint8_t address, std::uint64_t value) {
    if (address == GsPrim) {
        prim_ = value;
        // Avoid mixing vertices from different primitive modes.
        if ((prim_ & PrimitiveTypeMask) != TriangleList) triangle_vertex_count_ = 0;
        return;
    }
    if (address == GsRgbaq) {
        current_color_ = unpack_rgba(value);
        return;
    }
    if (address != GsXyz2 || (prim_ & PrimitiveTypeMask) != TriangleList) return;

    // GS screen coordinates are 12.4 fixed point. This first renderer slice
    // uses framebuffer pixels directly; XYOFFSET, scissor GS registers, Z,
    // textures, blending, and other primitive modes are not implemented yet.
    const auto xFixed = static_cast<std::uint16_t>(value & 0xFFFFU);
    const auto yFixed = static_cast<std::uint16_t>((value >> 16U) & 0xFFFFU);
    const double x = static_cast<double>(xFixed) / 16.0;
    const double y = static_cast<double>(yFixed) / 16.0;
    triangle_vertices_[triangle_vertex_count_] = {x, y, current_color_};
    ++triangle_vertex_count_;
    if (triangle_vertex_count_ == triangle_vertices_.size()) {
        if (framebuffer_.draw_triangle(triangle_vertices_[0], triangle_vertices_[1],
                                       triangle_vertices_[2])) {
            ++triangles_rendered_;
        }
        triangle_vertex_count_ = 0;
    }
}

const GsRegisterState& GsCommandProcessor::state() const { return state_; }
GsRegisterState& GsCommandProcessor::state() { return state_; }
const GsFramebuffer& GsCommandProcessor::framebuffer() const { return framebuffer_; }
GsFramebuffer& GsCommandProcessor::framebuffer() { return framebuffer_; }
const std::string& GsCommandProcessor::error() const { return error_; }
std::uint64_t GsCommandProcessor::packets_processed() const { return packets_processed_; }
std::uint64_t GsCommandProcessor::writes_processed() const { return writes_processed_; }
std::uint64_t GsCommandProcessor::triangles_rendered() const { return triangles_rendered_; }

void GsCommandProcessor::reset() {
    decoder_.clear();
    state_.reset();
    framebuffer_.clear({0, 0, 0, 0});
    framebuffer_.reset_scissor();
    error_.clear();
    packets_processed_ = 0;
    writes_processed_ = 0;
    triangles_rendered_ = 0;
    prim_ = 0;
    current_color_ = {255, 255, 255, 255};
    triangle_vertex_count_ = 0;
}

} // namespace retroshelf
