#include "bolt.h"

using namespace BOLT;


// Decompress algorithm used by N64 and GBA games. (entirely guessed)
void bolt_reader_t::decompress_n64(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result) {
  set_cur_pos(offset);

  std::uint32_t op_count = 0;
  std::uint32_t ext_offset = 0;
  std::uint32_t ext_run = 0;

  while (result.size() < expected_size) {
    std::uint8_t bytevalue = static_cast<std::uint8_t>(read_u8());
    op_count++;

    if (bytevalue & 0x80) {
      if (bytevalue & 0x40) {  // extension in offset
        ext_offset <<= 6;
        ext_offset |= bytevalue & 0x3F;
      }
      else if (bytevalue & 0x20) {  // extension in runlength
        ext_run <<= 5;
        ext_run |= bytevalue & 0x1F;
      }
      else if (bytevalue & 0x10) { // extension in both runlength and offset
        ext_run <<= 2;
        ext_offset <<= 2;

        ext_offset |= (bytevalue & 0b1100) >> 2;
        ext_run |= (bytevalue & 0b0011);
      }
      else { // uncompressed
        std::uint32_t run_length = ((ext_run << 4) | (bytevalue & 0xF)) + 1;
        for (unsigned i = 0; i < run_length; ++i) {
          result.push_back(read_u8());
        }
        op_count = ext_offset = ext_run = 0;
      }
    }
    else {  // lookup
      if (result.size() == 0) {
        err_msg("lookup can't happen on first byte, something fishy is going on", bytevalue);
        break;
      }

      std::int32_t target_offset = std::int32_t(result.size()) - 1 - ((ext_offset << 4) | (bytevalue & 0xF));
      std::uint32_t run_length = ((ext_run << 3) | (bytevalue >> 4)) + op_count + 1;

      if (target_offset < 0) {
        err_msg("negative value for lookbehind", bytevalue);
        break;
      }
      if (result.size() <= target_offset) {
        err_msg("lookbehind too far", bytevalue);
        break;
      }

      for (unsigned i = 0; i < run_length; ++i) {
        result.push_back(result[target_offset + i]);
      }
      op_count = ext_offset = ext_run = 0;
    }
  }
}
