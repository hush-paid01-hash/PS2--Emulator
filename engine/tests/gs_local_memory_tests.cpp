#include "retroshelf/gs_local_memory.hpp"

#include <array>
#include <cstdint>
#include <iostream>

int main() {
    int failures = 0;
    auto check = [&](bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    retroshelf::GsLocalMemory memory;
    check(memory.bytes().size() == retroshelf::GsLocalMemory::SizeBytes,
          "allocates the 4 MiB GS local-memory region");
    check(memory.read_byte(0) == 0 && memory.read_byte(0x1234U) == 0,
          "initializes local memory to zero");

    const std::array<std::uint8_t, 4> source{{0x11, 0x22, 0x33, 0x44}};
    check(memory.write(0x100U, source.data(), source.size()), "writes a byte range");
    std::array<std::uint8_t, 4> destination{};
    check(memory.read(0x100U, destination.data(), destination.size()), "reads a byte range");
    check(destination == source, "preserves byte order on readback");

    const std::array<std::uint8_t, 3> edge{{0xA1, 0xB2, 0xC3}};
    const auto end = static_cast<std::uint32_t>(retroshelf::GsLocalMemory::SizeBytes - 1U);
    check(memory.write(end, edge.data(), edge.size()), "wraps a write across local-memory end");
    std::array<std::uint8_t, 3> edge_read{};
    check(memory.read(end, edge_read.data(), edge_read.size()), "wraps a read across local-memory end");
    check(edge_read == edge, "preserves bytes across wrapped address");
    check(memory.read_byte(0) == 0xB2 && memory.read_byte(1) == 0xC3,
          "wrapped bytes appear at the beginning of local memory");

    check(memory.fill(0x200U, 0x5A, 8), "fills a bounded range");
    check(memory.read_byte(0x207U) == 0x5A, "fill reaches final requested byte");
    check(!memory.fill(0, 0, retroshelf::GsLocalMemory::SizeBytes + 1U),
          "rejects fill larger than the whole memory");
    check(!memory.read(0, nullptr, 1), "rejects null destination for non-empty read");
    check(!memory.write(0, nullptr, 1), "rejects null source for non-empty write");

    memory.reset();
    check(memory.read_byte(0) == 0 && memory.read_byte(0x100U) == 0 &&
              memory.read_byte(end) == 0,
          "reset clears local memory including wrapped addresses");

    return failures == 0 ? 0 : 1;
}
