#pragma once
#include <vector>


namespace BOLT {
  extern bool g_big_endian;
}

inline void reinsert_self(std::vector<std::byte>& result, unsigned rel_offset, unsigned run_length) {
  for (unsigned i = 0; i < run_length; i++) {
    std::byte v = result[result.size() - rel_offset];
    result.push_back(v);
  }
}

inline std::uint32_t bswap_if(std::uint32_t v) {
  if (BOLT::g_big_endian) {
    return
      ((v & 0x000000FF) << 24) |
      ((v & 0x0000FF00) << 8) |
      ((v & 0x00FF0000) >> 8) |
      ((v & 0xFF000000) >> 24);
  }
  return v;
}

inline std::uint16_t bswap_if(std::uint16_t v) {
  if (BOLT::g_big_endian) {
    return
      ((v & 0x00FF) << 8) |
      ((v & 0xFF00) >> 8);
  }
  return v;
}

