#include "bolt.h"
#include "util.h"

using namespace BOLT;


void bolt_reader_t::decompress_cdi(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result) {
  set_cur_pos(offset);

  while (result.size() < expected_size) {
    std::uint8_t bytevalue = static_cast<std::uint8_t>(read_u8());

    switch (bytevalue >> 4) {
    case 0x0:
    case 0x1: {
      for (unsigned i = 0; i < unsigned(bytevalue & 0x1F) + 1; ++i) {
        result.push_back(read_u8());
      }
      break;
    }
    case 0x2: {
      unsigned run_length = (bytevalue & 0xF) + 1;
      result.insert(result.end(), run_length, std::byte(0));
      break;
    }
    case 0x3: {
      std::byte b = read_u8();
      unsigned run_length = (bytevalue & 0xF) + 3;
      result.insert(result.end(), run_length, b);
      break;
    }
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7: {
      unsigned run_length = (bytevalue & 0x7) + 2;
      unsigned rel_offset = ((bytevalue >> 3) & 7) + 1;
      reinsert_self(result, rel_offset, run_length);
      break;
    }
    case 0x8: {
      std::uint8_t ext = std::uint8_t(read_u8());

      unsigned run_length = (ext & 0x3f) + 3;
      unsigned rel_offset = ((((bytevalue << 8) | ext) >> 6) & 0x3f) + 1;
      reinsert_self(result, rel_offset, run_length);
      break;
    }
    case 0x9: {
      std::uint8_t ext = std::uint8_t(read_u8());

      unsigned run_length = (ext & 0x3) + 3;
      unsigned rel_offset = ((((bytevalue << 8) | ext) >> 2) & 0x3ff) + 1;
      reinsert_self(result, rel_offset, run_length);
      break;
    }
    case 0xA: {
      std::uint8_t ext = std::uint8_t(read_u8());
      std::uint8_t ext2 = std::uint8_t(read_u8());

      unsigned run_length = ((ext << 8) | ext2);
      unsigned rel_offset = (bytevalue & 0xf) + 1;
      reinsert_self(result, rel_offset, run_length);
      break;
    }
    case 0xB: {
      std::uint8_t ext = std::uint8_t(read_u8());
      std::uint8_t ext2 = std::uint8_t(read_u8());

      unsigned run_length = (((ext & 0x3) << 8) | ext2) + 4;
      unsigned rel_offset = (((((ext & 0xff) << 8) | (bytevalue << 16)) >> 10) & 0x3ff) + 1;
      reinsert_self(result, rel_offset, run_length);
      break;
    }
    case 0xC:
    case 0xD: { // reverse nonsense
      unsigned run_length = (bytevalue & 0x3) + 2;
      unsigned rel_offset = (bytevalue >> 2) & 7;
      // TODO simplify
      for (unsigned i = 0; i < run_length; i++) {
        rel_offset++;
        std::byte v = result[result.size() - rel_offset];
        result.push_back(v);
        rel_offset++;
      }
      break;
    }
    case 0xE: { // reverse nonsense
      std::uint8_t ext = std::uint8_t(read_u8());

      unsigned run_length = (ext & 0x3f) + 3;
      unsigned rel_offset = (((bytevalue << 8) | ext) >> 6) & 0x3f;
      // TODO simplify
      for (unsigned i = 0; i < run_length; i++) {
        rel_offset++;
        std::byte v = result[result.size() - rel_offset];
        result.push_back(v);
        rel_offset++;
      }
      break;
    }
    case 0xF: { // reverse nonsense
      std::uint8_t ext = std::uint8_t(read_u8());
      std::uint8_t ext2 = std::uint8_t(read_u8());

      unsigned run_length = (((ext & 0x3) << 8) | ext2) + 4;
      unsigned rel_offset = ((((ext & 0xff) << 8) | (bytevalue << 16)) >> 10) & 0x3ff;
      // TODO simplify
      for (unsigned i = 0; i < run_length; i++) {
        rel_offset++;
        std::byte v = result[result.size() - rel_offset];
        result.push_back(v);
        rel_offset++;
      }
      break;
    }
    }
  }
}
