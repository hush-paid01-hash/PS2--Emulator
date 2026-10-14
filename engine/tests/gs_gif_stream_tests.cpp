#include "retroshelf/gs_gif_stream.hpp"
#include <cstdint>
#include <iostream>
#include <vector>
int main() {
  std::vector<retroshelf::gs::GifPacketView> packets;
  std::size_t consumed = 0;
  const std::uint64_t words[] = {1ULL | (1ULL << 60U), 0ULL, 0ULL, 0ULL, 1ULL << 15U | 2ULL << 58U, 0ULL};
  if (!retroshelf::gs::parse_gif_stream(words, 3, packets, consumed)) return 1;
  if (packets.size() != 2 || packets[0].payload_qwords != 1 || consumed != 3) return 2;
  if (retroshelf::gs::parse_gif_stream(words, 2, packets, consumed)) return 3;
  if (retroshelf::gs::parse_gif_stream(nullptr, 0, packets, consumed)) return 4;
  std::cout << "GIF stream tests passed\n";
  return 0;
}
