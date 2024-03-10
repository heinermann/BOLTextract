#pragma once
#include <vector>


inline void reinsert_self(std::vector<std::byte>& result, unsigned rel_offset, unsigned run_length) {
  for (unsigned i = 0; i < run_length; i++) {
    std::byte v = result[result.size() - rel_offset];
    result.push_back(v);
  }
}
