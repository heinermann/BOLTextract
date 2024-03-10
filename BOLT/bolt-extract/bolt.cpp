#include <string>
#include <filesystem>
#include <fstream>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <cstddef>
#include <sstream>
#include <iostream>

#include "bolt.h"
#include "guess_type.h"

using namespace BOLT;

namespace BOLT {
  bool g_big_endian = false;
}

std::uint32_t BOLT::bswap_if(std::uint32_t v) {
  if (g_big_endian) {
    return
      ((v & 0x000000FF) << 24) |
      ((v & 0x0000FF00) << 8) |
      ((v & 0x00FF0000) >> 8) |
      ((v & 0xFF000000) >> 24);
  }
  return v;
}

uint32_t entry_t::uncompressed_size() const {
  return bswap_if(uncompressed_size_be);
}

uint32_t entry_t::data_offset() const {
  return bswap_if(data_offset_be);
}

uint32_t entry_t::file_hash() const {
  return bswap_if(file_hash_be);
}

void bolt_reader_t::read_from_file(const std::filesystem::path& filename) {
  std::ifstream rom_file(filename, std::ios::binary | std::ios::ate);
  std::streamsize rom_size = rom_file.tellg();
  rom_file.seekg(0);

  rom.resize(rom_size);
  rom_file.read(reinterpret_cast<char*>(rom.data()), rom_size);

  find_bolt_archive();
}

constexpr std::byte BOLT_STR[] = { std::byte('B'), std::byte('O'), std::byte('L'), std::byte('T') };
constexpr std::byte BOLT_STR_LOWER[] = { std::byte('b'), std::byte('o'), std::byte('l'), std::byte('t') };

void bolt_reader_t::find_bolt_archive() {
  auto found = std::ranges::search(rom, BOLT_STR);
  if (!found) {
    found = std::ranges::search(rom, BOLT_STR_LOWER);
    if (!found) {
      throw std::runtime_error("Failed to find BOLT header. Rom is either incorrect format, corrupted, or does not contain BOLT archive.");
    }
  }

  bolt_begin = cursor_pos = std::distance(rom.begin(), found.begin());
  archive = reinterpret_cast<archive_t*>(&rom[bolt_begin]);
}

void bolt_reader_t::set_cur_pos(std::size_t pos) {
  cursor_pos = bolt_begin + pos;
}

std::byte bolt_reader_t::read_u8() {
  std::byte v = rom[cursor_pos];
  cursor_pos++;
  return v;
}

void bolt_reader_t::extract_all_to(const std::filesystem::path& out_dir) {
  unsigned num_entries = this->archive->num_entries;
  if (num_entries == 0) num_entries = 256;

  for (unsigned i = 0; i < num_entries; ++i) {
    extract_entry(out_dir, archive->entries[i]);
  }
}

void bolt_reader_t::extract_dir(const std::filesystem::path& out_dir, const entry_t* entries, std::uint32_t num_entries) {
  for (std::uint32_t i = 0; i < num_entries; ++i) {
    extract_entry(out_dir, entries[i]);
  }
}

void bolt_reader_t::extract_entry(const std::filesystem::path& out_dir, const entry_t& entry) {
  std::uint32_t hash = entry.file_hash();
  std::uint32_t offset = entry.data_offset();

  if (hash == 0) {  // is directory
    std::uint32_t num_items = entry.file_type;
    if (num_items == 0) num_items = 256;

    std::ostringstream ss;
    ss << std::hex << offset;
    extract_dir(out_dir / ss.str(), entry_at(offset), num_items);
  }
  else { // is file
    extract_file(out_dir, entry);
  }
}

void bolt_reader_t::err_msg(const std::string& msg, std::uint8_t value) {
  std::cerr << msg << "; value " << std::uint32_t(value) << " at offset " << std::hex << cursor_pos << " (BOLT+" << (cursor_pos - bolt_begin) << "); Filetype: " << std::uint32_t(current_filetype) << "\n";
}

void bolt_reader_t::extract_file(const std::filesystem::path& out_dir, const entry_t& entry) {
  std::uint32_t expected_size = entry.uncompressed_size();
  std::uint32_t hash = entry.file_hash();
  std::uint32_t offset = entry.data_offset();

  std::vector<std::byte> result;
  result.reserve(expected_size);

  this->current_filetype = entry.file_type;

  if (entry.flags & FLAG_UNCOMPRESSED) {
    set_cur_pos(offset);
    result.insert(result.end(), &rom[cursor_pos], &rom[cursor_pos] + expected_size);
  }
  else {
    switch (algorithm) {
    case algorithm_t::CDI:
      decompress_cdi(offset, expected_size, result);
      break;
    case algorithm_t::DOS:
      decompress_dos(offset, expected_size, result);
      break;
    case algorithm_t::N64:
      decompress_n64(offset, expected_size, result);
      break;
    case algorithm_t::WIN:
      decompress_win(offset, expected_size, result);
      break;
    }
  }

  write_result(out_dir, hash, result, expected_size);
}

void bolt_reader_t::write_result(const std::filesystem::path& base_dir, std::uint32_t hash, const std::vector<std::byte>& data, std::uint32_t filesize) {
  std::ostringstream filehash;
  filehash << std::hex << hash << guess_extension(data);
  std::filesystem::path filename = base_dir / filehash.str();

  if (data.size() != filesize) {
    std::cerr << "Result size is wrong. " << std::dec << data.size() << " != " << filesize << " for file " << filename.filename() << "\n";
  }

  std::filesystem::create_directories(base_dir);
  std::ofstream ofile(filename, std::ios::binary);
  ofile.write(reinterpret_cast<const char*>(data.data()), data.size());
}

const entry_t* bolt_reader_t::entry_at(std::uint32_t offset) const {
  return reinterpret_cast<const entry_t*>(&rom[bolt_begin + offset]);
}

bool BOLT::extract_bolt(const std::filesystem::path& input_file, const std::filesystem::path& output_dir, algorithm_t algorithm) {
  std::filesystem::create_directories(output_dir);

  bolt_reader_t reader{ algorithm };
  reader.read_from_file(input_file);
  reader.extract_all_to(output_dir);
  return true;
}

bolt_reader_t::bolt_reader_t(algorithm_t algo)
  : algorithm(algo) {}

