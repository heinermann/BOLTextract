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

std::uint32_t BOLT::bswap(std::uint32_t v) {
  return
    ((v & 0x000000FF) << 24) |
    ((v & 0x0000FF00) << 8) |
    ((v & 0x00FF0000) >> 8) |
    ((v & 0xFF000000) >> 24);
}

uint32_t entry_t::flags() const {
  //if (g_big_endian)
    return bswap(flags_be);
  //return flags_be;
}

uint32_t entry_t::uncompressed_size() const {
  if (g_big_endian)
    return bswap(uncompressed_size_be);
  return uncompressed_size_be;
}

uint32_t entry_t::data_offset() const {
  if (g_big_endian)
    return bswap(data_offset_be);
  return data_offset_be;
}

uint32_t entry_t::name_hash() const {
  if (g_big_endian)
    return bswap(name_hash_be);
  return name_hash_be;
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
  std::uint32_t earliest_ref = std::numeric_limits<std::uint32_t>::max();
  for (int i = 0; offsetof(archive_t, entries[i]) < earliest_ref; ++i) {
    
    uint32_t data_offset = archive->entries[i].data_offset();
    if (data_offset == 0) continue;
    if (data_offset < earliest_ref) earliest_ref = data_offset;

    extract_entry(out_dir, archive->entries[i]);
  }
}

void bolt_reader_t::extract_dir(const std::filesystem::path& out_dir, const entry_t* entries, std::uint32_t num_entries) {
  for (std::uint32_t i = 0; i < num_entries; ++i) {
    extract_entry(out_dir, entries[i]);
  }
}

void bolt_reader_t::extract_entry(const std::filesystem::path& out_dir, const entry_t& entry) {
  std::uint32_t flags = entry.flags();
  std::uint32_t hash = entry.name_hash();
  std::uint32_t offset = entry.data_offset();

  if (hash == 0) {  // is directory
    std::uint32_t num_items = flags & 0xFF;
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
  std::cerr << msg << "; value " << std::uint32_t(value) << " at offset " << std::hex << cursor_pos << " (BOLT+" << (cursor_pos - bolt_begin) << ")\n";
}

void bolt_reader_t::decompress(std::uint32_t offset, std::uint32_t expected_size, std::vector<std::byte>& result) {
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
    /*else if (result.size() == 0) {
      for (unsigned i = 0; i < bytevalue; ++i) {
        result.push_back(read_u8());
      }
    }*/
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

void bolt_reader_t::extract_file(const std::filesystem::path& out_dir, const entry_t& entry) {
  std::uint32_t flags = entry.flags();
  std::uint32_t expected_size = entry.uncompressed_size();
  std::uint32_t hash = entry.name_hash();
  std::uint32_t offset = entry.data_offset();

  std::vector<std::byte> result;
  result.reserve(expected_size);

  if (flags & FLAG_UNCOMPRESSED) {
    set_cur_pos(offset);
    result.insert(result.end(), &rom[cursor_pos], &rom[cursor_pos] + expected_size);
  }
  else {
    decompress(offset, expected_size, result);
  }

  write_result(out_dir, hash, result, expected_size);
}

void bolt_reader_t::write_result(const std::filesystem::path& base_dir, std::uint32_t hash, const std::vector<std::byte>& data, std::uint32_t filesize) {
  std::ostringstream filehash;
  filehash << std::hex << hash << guess_extension(data);
  std::filesystem::path filename = base_dir / filehash.str();

  if (data.size() != filesize) {
    std::cerr << "Result size is wrong. " << std::dec << data.size() << " != " << filesize << " for file " << filename << "\n";
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
