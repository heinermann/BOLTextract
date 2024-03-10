#include "bolt.h"
#include "util.h"

using namespace BOLT;


// Decompress algorithm used by The Game of Life and ???.
void bolt_reader_t::decompress_win(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result) {
  set_cur_pos(offset);

  while (result.size() < expected_size) {
    std::uint8_t bytevalue = static_cast<std::uint8_t>(read_u8());

    switch (bytevalue >> 4) {
    case 0x0:
      if (bytevalue) {
        for (unsigned i = 0; i < bytevalue; ++i) {
          result.push_back(read_u8());
        }
        continue;
      }

      if (result.size() != expected_size) {
        std::ostringstream ss;
        ss << "finished decompression with invalid size; Expected size: " << expected_size << "; Got: " << result.size();
        err_msg(ss.str(), bytevalue);
      }
      return;
    case 0x1: {
      std::byte v = result[result.size() - ((bytevalue & 0xF) + 9)];
      result.push_back(v);
      result.push_back(v);
      break;
    }
    case 0x2:
    case 0x3: {
      std::uint8_t b2 = std::uint8_t(read_u8());
      unsigned run_length = (bytevalue & 0xF) + 3;
      unsigned rel_offset = 2 * b2 + ((bytevalue >> 4) & 1);
      reinsert_self(result, rel_offset, run_length);
      break;
    }
    case 0x4: {
      std::byte repeat_byte = read_u8();
      unsigned run_length = (bytevalue & 0xF) + 3;
      result.insert(result.end(), run_length, repeat_byte);
      break;
    }
    case 0x5: {
      std::uint8_t b2 = std::uint8_t(read_u8());
      std::byte repeat_byte = read_u8();

      unsigned run_length = 4 * (16 * b2 + (bytevalue & 0xF)) + 19;
      result.insert(result.end(), run_length, repeat_byte);
      break;
    }
    case 0x6: {
      unsigned run_length = (bytevalue & 0xF) + 2;
      result.insert(result.end(), run_length, std::byte(0));
      break;
    }
    case 0x7:
    case 0x8:
    case 0x9:
    case 0xA:
    case 0xB:
      result.push_back(result[result.size() - (bytevalue - 103)]);
      result.push_back(result[result.size() - (bytevalue - 103)]);
      break;
    case 0xC:
    case 0xD:
    case 0xE:
    case 0xF: {
      result.push_back(result[result.size() - (((bytevalue & 0x38) >> 3) + 1)]);
      result.push_back(result[result.size() - ((bytevalue & 7) + 2)]);
      break;
    }
    }
  }
}

// The Game of Life filetype 0x09, DOS games have something similar for 0x08
void bolt_reader_t::decompress_win_special_9(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result) {
  decompress_win(offset, 24, result);
  // TODO multichunk entry
}
