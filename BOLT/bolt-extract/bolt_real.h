#pragma once
#include <cstdint>

// This is just for documentation purposes

typedef void* HANDLE;

struct BOLTEntryList;


struct BOLTFolder {
  std::uint8_t flags;
  std::uint8_t unk_1;
  std::uint8_t unk_2;
  std::uint8_t num_entries;
  std::uint32_t field_4;
  std::uint32_t data_offset;
  BOLTEntryList *entries;
};

struct BOLTEntry {
  std::uint8_t flags;
  std::uint8_t unk_1;
  std::uint8_t unk_2;
  std::uint8_t file_type;
  std::uint32_t uncompressed_size;
  std::uint32_t data_offset;
  std::uint32_t file_hash;
};

struct BOLTEntryList {
  std::uint32_t field_0;
  BOLTEntry entries[1];
};

typedef void* (__cdecl* t_openfn)(BOLTArchive*);
typedef BOLTEntry* (__cdecl* t_closefn)(BOLTArchive*);
typedef void (__cdecl* t_unknownfn)(BOLTArchive*);

struct BOLTAccess {
  t_openfn* open_fns;
  t_closefn* close_fns;
  t_unknownfn* field_8;
  t_unknownfn* field_C;
  t_unknownfn* field_10;
  t_unknownfn* field_14;
};

struct BOLTArchive {
  std::uint16_t small_bolt; // boolean if the archive is lowercase 'bolt'
  std::uint16_t num_instances;  // number of times archive was opened
  std::uint16_t field_4;
  std::uint16_t field_6;
  
  HANDLE h_file;
  HANDLE h_map;
  void* p_file_data;

  BOLTFolder* current_folder;
  BOLTEntry* current_entry;
  BOLTEntryList* current_entry_list;
  std::uint32_t current_idx;

  BOLTAccess access_class;
  
  BOLTFolder entries[1];
};
