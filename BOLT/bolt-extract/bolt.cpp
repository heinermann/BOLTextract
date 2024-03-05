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

// DOS games and MAYBE CD-i?
void bolt_reader_t::decompress_v1(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result) {
  // TODO
}

// Decompress algorithm used by N64 and GBA games. (entirely guessed)
void bolt_reader_t::decompress_v2(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result) {
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

// Decompress algorithm used by The Game of Life and ???.
void bolt_reader_t::decompress_v3(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result) {
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

      // TODO simplify
      for (unsigned i = 0; i < run_length; i++) {
        std::byte v = result[result.size() - rel_offset];
        result.push_back(v);
      }
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
void bolt_reader_t::decompress_v3_special_9(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result) {
  // TODO multichunk entry
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
    decompress_v2(offset, expected_size, result);
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

bool BOLT::extract_bolt(const std::filesystem::path& input_file, const std::filesystem::path& output_dir) {
  std::filesystem::create_directories(output_dir);

  bolt_reader_t reader;
  reader.read_from_file(input_file);
  reader.extract_all_to(output_dir);
  return true;
}
