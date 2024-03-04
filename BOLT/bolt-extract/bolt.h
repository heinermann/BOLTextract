#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace BOLT {
  bool extract_bolt(const std::string& input_file, const std::string& output_dir);

  std::uint32_t bswap(std::uint32_t v);

  extern bool g_big_endian;

  /*
  Flag notes:
  0x0A - seen for most files (txt, fnt, chk)
  0x0E - seen for few files
  0x12 - seen for few files

  0x19 - seen for many files
  0x1A - seen for many files
  0x1C - seen for few files

  */
  enum flags_t {
    FLAG_UNCOMPRESSED = 0x8000000
  };
  
  struct entry_t {
    std::uint32_t flags_be;
    std::uint32_t uncompressed_size_be;
    std::uint32_t data_offset_be;
    std::uint32_t name_hash_be;

    uint32_t flags() const;
    uint32_t uncompressed_size() const;
    uint32_t data_offset() const;
    uint32_t name_hash() const;
  };

  struct archive_t {
    uint32_t magic; // 'B', 'O', 'L', 'T'
    uint32_t unk1;
    uint32_t unk2;
    uint32_t end_offset;
    entry_t entries[1];
  };

  class bolt_reader_t {
  private:
    std::vector<std::byte> rom;

    std::size_t bolt_begin = 0;
    std::size_t cursor_pos = 0;

    const archive_t* archive;

    const entry_t* entry_at(std::uint32_t offset) const;

    std::byte read_u8();
    void err_msg(const std::string& msg, std::uint8_t opcode);

    void extract_dir(const std::string& out_dir, const entry_t *entries, uint32_t num_entries);
    void extract_file(const std::string& out_dir, const entry_t& entry);
    void extract_entry(const std::string& out_dir, const entry_t& entry);

    void set_cur_pos(std::size_t pos);

    void find_bolt_archive();
    void write_result(const std::string& base_dir, std::uint32_t hash, const std::vector<std::byte> &data, std::uint32_t filesize);
    void decompress(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result);
  public:
    void read_from_file(const std::string& filename);
    void extract_all_to(const std::string& out_dir);
  };
}