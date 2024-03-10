#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <filesystem>


namespace BOLT {
  enum class algorithm_t {
    UNKNOWN,
    CDI,
    DOS,
    N64,
    WIN,
    XBOX,
  };

  bool extract_bolt(const std::filesystem::path& input_file, const std::filesystem::path& output_dir, algorithm_t algorithm);

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
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t millisecond;
    uint8_t month;
    uint8_t day;
    uint8_t year;   // Since 1900
    uint8_t num_entries;
    uint32_t end_offset;  // most of the time
    entry_t entries[1];
  };

  struct archive_t_xbox {
    uint32_t magic; // 'B', 'O', 'L', 'T'
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t month;
    uint8_t day;
    uint8_t year;   // Since 1900
    uint16_t num_entries;
    uint32_t end_offset;  // most of the time
    entry_t entries[1];
  };

  class bolt_reader_t {
  private:
    std::vector<std::byte> rom;

    algorithm_t algorithm;

    std::size_t bolt_begin = 0;
    std::size_t cursor_pos = 0;

    std::uint8_t current_filetype = 255;

    const archive_t* archive;

    const entry_t* entry_at(std::uint32_t offset) const;

    std::byte read_u8();
    void err_msg(const std::string& msg, std::uint8_t opcode);

    void extract_dir(const std::filesystem::path& out_dir, const entry_t *entries, uint32_t num_entries);
    void extract_file(const std::filesystem::path& out_dir, const entry_t& entry, unsigned index);
    void extract_entry(const std::filesystem::path& out_dir, const entry_t& entry, unsigned index);

    void set_cur_pos(std::size_t pos);

    void find_bolt_archive();
    void write_result(const std::filesystem::path& base_dir, unsigned index, const std::vector<std::byte> &data, std::uint32_t filesize);
    void decompress_cdi(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result);
    void decompress_dos(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result);
    void decompress_n64(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result);
    void decompress_win(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result);
    void decompress_win_special_9(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result);
    void decompress_dos_special_8(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result);

    unsigned get_num_entries();
  public:
    void read_from_file(const std::filesystem::path& filename);
    void extract_all_to(const std::filesystem::path& out_dir);

    bolt_reader_t(algorithm_t algo);
  };
}