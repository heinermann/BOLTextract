#include <string>
#include <array>
#include <vector>
#include <cstddef>
#include <cctype>

#include "guess_type.h"

bool check_easy_header(const std::vector<std::byte>& d, char c1, char c2, char c3, char c4) {
  return d.size() > 32 &&
    d[0] == std::byte(c1) &&
    d[1] == std::byte(c2) &&
    d[2] == std::byte(c3) &&
    d[3] == std::byte(c4);
}

bool is_wav_file(const std::vector<std::byte>& data) {
  return check_easy_header(data, 'R', 'I', 'F', 'F');
}

bool is_txt_file(const std::vector<std::byte>& data) {
  for (std::byte b : data) {
    unsigned char c = static_cast<unsigned char>(b);
    if (!std::isprint(c) && !std::isspace(c) && c != '‘' && c != '’' && c != '“' && c != '”') return false;
  }
  return true;
}

struct img_header_t { // big endian header
  std::uint16_t unk1;
  std::uint16_t bpp;
  std::uint32_t unk2;
  std::uint16_t width;
  std::uint16_t height;
  std::uint32_t unk4;
};

bool is_img_file(const std::vector<std::byte>& data) {
  if (data.size() <= sizeof(img_header_t)) return false;

  const img_header_t* tgabw = reinterpret_cast<const img_header_t*>(data.data());
  
  std::uint16_t width = ((tgabw->width & 0x00FF) << 8) | ((tgabw->width & 0xFF00) >> 8);
  std::uint16_t height = ((tgabw->height & 0x00FF) << 8) | ((tgabw->height & 0xFF00) >> 8);

  return 
    data.size() == width * height + sizeof(img_header_t) && 
    (tgabw->unk1 & 0xFF) == 0 &&
    ((tgabw->unk1 & 0xFF00) >> 8) < 5 &&
    tgabw->bpp == 0x0800 &&
    tgabw->unk2 == 0 && 
    tgabw->unk4 == 0;
}

struct pal_header_t { // big endian header
  std::uint32_t unk1;  // byte 4 has bpp
  std::uint16_t entries;
  std::uint16_t unk2;
};

bool is_pal_file(const std::vector<std::byte>& data) {
  if (data.size() <= sizeof(pal_header_t)) return false;
  const pal_header_t* pal = reinterpret_cast<const pal_header_t*>(data.data());

  return
    data.size() == 255 * 2 + sizeof(pal_header_t) &&
    pal->unk1 == 0 &&
    pal->entries == 0xFF00;
}

bool is_fnt_file(const std::vector<std::byte>& data) {
  return check_easy_header(data, 'F', 'O', 'N', 'T');
}

bool is_chk_file(const std::vector<std::byte>& data) {
  return
    check_easy_header(data, 'T', 'Y', 'P', 'E') ||
    check_easy_header(data, 'V', 'E', 'R', ' ') ||
    check_easy_header(data, 'I', 'V', 'E', 'R') ||
    check_easy_header(data, 'I', 'V', 'E', '2') ||
    check_easy_header(data, 'V', 'C', 'O', 'D');
}

std::string guess_extension(const std::vector<std::byte>& data) {
  if (is_wav_file(data)) return ".wav";
  else if (is_fnt_file(data)) return ".fnt";
  else if (is_chk_file(data)) return ".chk";
  else if (is_img_file(data)) return ".unkimg";
  else if (is_pal_file(data)) return ".unkpal";
  else if (is_txt_file(data)) return ".txt";
  else return ".unk";
}
