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
    if (!std::isprint(c) && !std::isspace(c)) return false;
  }
  return true;
}

struct pcx_header_t {
  std::uint8_t fixed;
  std::uint8_t version;
  std::uint8_t encoding;
  std::uint8_t bpp;
  std::uint16_t min_x;
  std::uint16_t min_y;
  std::uint16_t max_x;
  std::uint16_t max_y;
  std::uint16_t width_dpi;
  std::uint16_t height_dpi;
  std::array<std::uint8_t, 48> ega_palette;
  std::uint8_t reserved1;
  std::uint8_t colorplanes;
  std::uint16_t bytes_per_line;
  std::uint16_t palette_mode;
  std::uint16_t src_width;
  std::uint16_t src_height;
  std::array<std::uint8_t, 54> reserved2;
};

static_assert(sizeof(pcx_header_t) == 128);

bool is_pcx_file(const std::vector<std::byte>& data) {
  if (data.size() <= sizeof(pcx_header_t)) return false;

  const pcx_header_t* pcx = reinterpret_cast<const pcx_header_t*>(data.data());
  return
    pcx->fixed == 10 &&
    pcx->version <= 5 &&
    pcx->encoding <= 1 &&
    pcx->bpp <= 8 &&
    pcx->min_x <= pcx->max_x &&
    pcx->min_y <= pcx->max_y &&
    pcx->max_x > 0 &&
    pcx->max_y > 0 &&
    pcx->palette_mode <= 2;
}

bool is_fnt_file(const std::vector<std::byte>& data) {
  return check_easy_header(data, 'F', 'O', 'N', 'T');
}

bool is_smk_file(const std::vector<std::byte>& data) {
  return check_easy_header(data, 'S', 'M', 'K', '2');
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
  else if (is_smk_file(data)) return ".smk";
  else if (is_chk_file(data)) return ".chk";
  else if (is_pcx_file(data)) return ".pcx";
  else if (is_txt_file(data)) return ".txt";
  else return ".unk";
}
