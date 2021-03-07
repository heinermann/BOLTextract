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

std::uint32_t BOLT::bswap(std::uint32_t v) {
  return
    ((v & 0x000000FF) << 24) |
    ((v & 0x0000FF00) << 8) |
    ((v & 0x00FF0000) >> 8) |
    ((v & 0xFF000000) >> 24);
}

uint32_t entry_t::flags() const {
  return bswap(flags_be);
}

uint32_t entry_t::uncompressed_size() const {
  return bswap(uncompressed_size_be);
}

uint32_t entry_t::data_offset() const {
  return bswap(data_offset_be);
}

uint32_t entry_t::name_hash() const {
  return bswap(name_hash_be);
}

void bolt_reader_t::read_from_file(const std::string& filename) {
  std::ifstream rom_file(filename, std::ios::binary | std::ios::ate);
  std::streamsize rom_size = rom_file.tellg();
  rom_file.seekg(0);

  rom.resize(rom_size);
  rom_file.read(reinterpret_cast<char*>(rom.data()), rom_size);

  find_bolt_archive();
}

void bolt_reader_t::find_bolt_archive() {
  constexpr char boltStr[] = "BOLT";
  char* romptr = reinterpret_cast<char*>(rom.data());
  auto it = std::search(romptr, romptr + rom.size(), &boltStr[0], &boltStr[4]);
  if (it == romptr + rom.size()) throw std::runtime_error("Failed to find BOLT header. Rom is either incorrect format, corrupted, or does not contain BOLT archive.");
  
  bolt_begin = cursor_pos = std::distance(romptr, it);
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

void bolt_reader_t::extract_all_to(const std::string& out_dir) {
  std::uint32_t earliest_ref = std::numeric_limits<std::uint32_t>::max();
  for (int i = 0; offsetof(archive_t, entries[i]) < earliest_ref; ++i) {
    
    uint32_t data_offset = archive->entries[i].data_offset();
    if (data_offset < earliest_ref) earliest_ref = data_offset;

    extract_entry(out_dir, archive->entries[i]);
  }
}

void bolt_reader_t::extract_dir(const std::string& out_dir, const entry_t* entries, std::uint32_t num_entries) {
  for (std::uint32_t i = 0; i < num_entries; ++i) {
    extract_entry(out_dir, entries[i]);
  }
}

void bolt_reader_t::extract_entry(const std::string& out_dir, const entry_t& entry) {
  std::uint32_t flags = entry.flags();
  std::uint32_t hash = entry.name_hash();
  std::uint32_t offset = entry.data_offset();

  if (hash == 0) {  // is directory
    std::uint32_t num_items = flags & 0xFF;
    if (num_items == 0) num_items = 256;

    std::ostringstream ss;
    ss << out_dir << "/" << std::hex << offset;
    extract_dir(ss.str(), entry_at(offset), num_items);
  }
  else { // is file
    extract_file(out_dir, entry);
  }
}

void bolt_reader_t::err_msg(const std::string& msg, std::uint8_t value) {
  std::cerr << msg << "; value " << std::uint32_t(value) << " at offset " << std::hex << cursor_pos << " (BOLT+" << (cursor_pos - bolt_begin) << ")\n";
}

void bolt_reader_t::extract_file(const std::string& out_dir, const entry_t& entry) {
  std::uint32_t flags = entry.flags();
  std::uint32_t result_size = entry.uncompressed_size();
  std::uint32_t hash = entry.name_hash();
  std::uint32_t offset = entry.data_offset();
  
  set_cur_pos(offset);
  
  std::vector<std::byte> result_filedata;
  result_filedata.reserve(result_size);

  if (flags & FLAG_UNCOMPRESSED) {
    result_filedata.insert(result_filedata.end(), &rom[cursor_pos], &rom[cursor_pos] + result_size);
  }
  else {
    std::uint32_t op_count = 0;
    std::uint32_t ext_offset = 0;
    std::uint32_t ext_run = 0;

    while (result_filedata.size() < result_size) {
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
            result_filedata.push_back(read_u8());
          }
          op_count = ext_offset = ext_run = 0;
        }
      }
      else {  // lookup
        std::uint32_t target_offset = result_filedata.size() - 1 - ((ext_offset << 4) | (bytevalue & 0xF));
        std::uint32_t run_length = ((ext_run << 3) | (bytevalue >> 4)) + op_count + 1;

        if (result_filedata.size() <= target_offset) {
          err_msg("lookbehind too far", bytevalue);
          break;
        }

        for (unsigned i = 0; i < run_length; ++i) {
          result_filedata.push_back(result_filedata[target_offset + i]);
        }
        op_count = ext_offset = ext_run = 0;
      }
    }

    // Write final result
    std::ostringstream filename;
    filename << out_dir << "/" << std::hex << hash << guess_extension(result_filedata);

    if (result_filedata.size() != result_size) {
      std::cerr << "Result size is wrong. " << std::dec << result_filedata.size() << " != " << result_size << " for file " << filename.str() << "\n";
    }

    std::filesystem::create_directories(out_dir);
    std::ofstream ofile(filename.str(), std::ios::binary);
    ofile.write(reinterpret_cast<char*>(result_filedata.data()), result_filedata.size());
  }
}

const entry_t* bolt_reader_t::entry_at(std::uint32_t offset) const {
  return reinterpret_cast<const entry_t*>(&rom[bolt_begin + offset]);
}

bool BOLT::extract_bolt(const std::string& input_file, const std::string& output_dir) {
  std::filesystem::create_directories(output_dir);

  bolt_reader_t reader;
  reader.read_from_file(input_file);
  reader.extract_all_to(output_dir);
  return true;
}
