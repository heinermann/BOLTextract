#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <filesystem>


namespace BOLT {
  bool extract_bolt(const std::filesystem::path& input_file, const std::filesystem::path& output_dir);

  std::uint32_t bswap_if(std::uint32_t v);

  extern bool g_big_endian;


  enum flags_t {
    FLAG_UNCOMPRESSED = 0x08
  };
  
  struct entry_t {
    std::uint8_t flags;
    std::uint8_t unk_1;
    
    // If this is specified, calls an indexed special function which doesn't exist anywhere I've seen
    std::uint8_t unk_2;

    std::uint8_t file_type;
    std::uint32_t uncompressed_size_be;
    std::uint32_t data_offset_be;

    // cleared and used as the uncompressed file pointer in official implementations
    std::uint32_t file_hash_be;

    uint32_t uncompressed_size() const;
    uint32_t data_offset() const;
    uint32_t file_hash() const;
  };

  struct archive_t {
    uint32_t magic; // 'B', 'O', 'L', 'T'
    uint32_t unk1;
    uint8_t unk2;
    uint8_t unk3;
    uint8_t unk4;
    uint8_t num_entries;
    uint32_t end_offset;  // most of the time
    entry_t entries[1];
  };

  class bolt_reader_t {
  private:
    std::vector<std::byte> rom;

    std::size_t bolt_begin = 0;
    std::size_t cursor_pos = 0;

    std::uint8_t current_filetype = 255;

    const archive_t* archive;

    const entry_t* entry_at(std::uint32_t offset) const;

    std::byte read_u8();
    void err_msg(const std::string& msg, std::uint8_t opcode);

    void extract_dir(const std::filesystem::path& out_dir, const entry_t *entries, uint32_t num_entries);
    void extract_file(const std::filesystem::path& out_dir, const entry_t& entry);
    void extract_entry(const std::filesystem::path& out_dir, const entry_t& entry);

    void set_cur_pos(std::size_t pos);

    void find_bolt_archive();
    void write_result(const std::filesystem::path& base_dir, std::uint32_t hash, const std::vector<std::byte> &data, std::uint32_t filesize);
    void decompress_v1(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result);
    void decompress_v2(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result);
    void decompress_v3(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result);
    void decompress_v3_special_9(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result);
  public:
    void read_from_file(const std::filesystem::path& filename);
    void extract_all_to(const std::filesystem::path& out_dir);
  };
}