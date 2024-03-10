#include "bolt.h"
#include "util.h"

using namespace BOLT;


// DOS games
void bolt_reader_t::decompress_dos(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result) {
  set_cur_pos(offset);

  unsigned opcode = 0;
  unsigned run_length = 0;
  unsigned rel_offset = 0;
  std::byte repeat_byte = std::byte(0);

  bool skip_opcode = false;

  while (result.size() < expected_size) {
    if (!skip_opcode) {
      std::uint8_t bytevalue = static_cast<std::uint8_t>(read_u8());
      std::uint8_t amount = bytevalue & 0x1F;

      if ((bytevalue & 0xC0) == 0) {
        opcode = 0;

        run_length = 31 - amount;
      }
      else if ((bytevalue & 0xC0) == 0x40) {
        opcode = 1;

        run_length = 35 - amount;
        rel_offset = 8 * (bytevalue & 0x20) + unsigned(read_u8());
      }
      else if ((bytevalue & 0xC0) == 0x80) {
        opcode = 1;

        run_length = 4 * (32 - amount);
        if (bytevalue & 0x20) run_length += 2;

        rel_offset = 2 * unsigned(read_u8());
      }
      else {
        opcode = 2;

        if (bytevalue & 0x20) {
          run_length = 0;
        }
        else {
          std::uint8_t run = std::uint8_t(read_u8());
          read_u8();  // wtf
          repeat_byte = read_u8();

          run_length = 4 * (32 - amount + 32 * run);
        }
      }
    }

    unsigned op_run_len;
    unsigned remaining_size = expected_size - result.size();
    if (remaining_size < run_length) {
      skip_opcode = true;
      op_run_len = remaining_size;
      run_length -= remaining_size;
    }
    else {
      op_run_len = run_length;
      skip_opcode = false;
    }


    switch (opcode) {
    case 0:
      for (unsigned i = 0; i < op_run_len; ++i) {
        result.push_back(read_u8());
      }
      break;
    case 1:
      reinsert_self(result, rel_offset, run_length);
      break;
    case 2:
      result.insert(result.end(), op_run_len, repeat_byte);
      break;
    }
  }
}

#pragma pack(push, 1)
struct Special8 {
  std::uint16_t field_0;
  std::uint16_t field_2;
  std::uint16_t field_4;
  std::uint16_t field_8;
  std::uint16_t field_A;
  std::uint16_t field_C;
  std::uint16_t field_E;
  std::uint16_t field_10;
  std::uint32_t field_12;
  std::uint16_t field_16;
};
#pragma pack(pop)

void bolt_reader_t::decompress_dos_special_8(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result) {
  decompress_dos(offset, 24, result);
}
