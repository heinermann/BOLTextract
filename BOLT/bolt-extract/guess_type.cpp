#include <string>
#include <array>
#include <vector>
#include <cstddef>
#include <cctype>

#include "guess_type.h"

namespace BOLT {
  extern bool g_big_endian;
}

std::uint32_t bswap_if(std::uint32_t v) {
  if (!BOLT::g_big_endian) return v;
  return
    ((v & 0x000000FF) << 24) |
    ((v & 0x0000FF00) << 8) |
    ((v & 0x00FF0000) >> 8) |
    ((v & 0xFF000000) >> 24);
}

std::uint16_t bswap_if(std::uint16_t v) {
  if (!BOLT::g_big_endian) return v;
  return
    ((v & 0x00FF) << 8) |
    ((v & 0xFF00) >> 8);
}


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

struct TStrTbl {
  std::uint16_t wStrCount;
  std::uint16_t wStrOffsets[1];
};

bool is_tbl_file(const std::vector<std::byte>& data) {
  if (data.size() <= sizeof(TStrTbl)) return false;
  const TStrTbl* pTbl = reinterpret_cast<const TStrTbl*>(data.data());
  
  if (pTbl->wStrCount <= 1) return false; // has data

  std::uint32_t data_start = pTbl->wStrCount * 2 + 2;
  if (data_start >= data.size()) return false; // in bounds
  if (pTbl->wStrOffsets[0] != data_start) return false; // expected offset
  
  for (unsigned i = 0; i < pTbl->wStrCount; i++) {
    if (pTbl->wStrOffsets[i] < data_start || pTbl->wStrOffsets[i] >= data.size()) return false; // in bounds
    if (i > 0 && data.at(pTbl->wStrOffsets[i] - 1) != std::byte(0)) return false; // null terminated string

    // must have incremental offsets (not a requirement, but a pattern)
    if (i > 0 && pTbl->wStrOffsets[i] <= pTbl->wStrOffsets[i - 1]) return false;
  }
  return data.back() == std::byte(0); // last null terminated string
}

struct FRAME {
  std::uint8_t dx;
  std::uint8_t dy;
  std::uint8_t wdt;
  std::uint8_t hgt;
  std::uint32_t offset;
};

#pragma pack(1)
struct GROUP {
  std::uint16_t wFrames;
  std::uint16_t wdt;
  std::uint16_t hgt;
  FRAME frames[1];
};
#pragma pack()

bool is_grp_file(const std::vector<std::byte>& data) {
  if (data.size() <= sizeof(GROUP)) return false;
  const GROUP* pGrp = reinterpret_cast<const GROUP*>(data.data());

  if (pGrp->wFrames == 0 || pGrp->wdt == 0 || pGrp->hgt == 0) return false; // has data

  std::uint32_t data_start = sizeof(GROUP) + (pGrp->wFrames - 1) * sizeof(FRAME);
  if (data_start + 1 >= data.size()) return false; // in bounds
  if (pGrp->frames[0].offset != data_start) return false; // expected first offset
  
  for (unsigned i = 0; i < pGrp->wFrames; i++) {
    if (pGrp->frames[i].wdt == 0) return false;
    if (pGrp->frames[i].hgt == 0) return false;

    if (pGrp->frames[i].dx + pGrp->frames[i].wdt > pGrp->wdt) return false;
    if (pGrp->frames[i].dy + pGrp->frames[i].hgt > pGrp->hgt) return false;

    if (pGrp->frames[i].offset < data_start || pGrp->frames[i].offset >= data.size()) return false;
  }
  return true;
}

#pragma pack(1)
struct MASSMEDIA_AUDIO {
  std::uint8_t channels;
  std::uint8_t bits;
  std::uint16_t sampleRate;
  std::uint32_t dataSize;
  std::uint32_t unknown;  // always 0?
};
#pragma pack()

bool is_audio_file(const std::vector<std::byte>& data) {
  if (data.size() <= sizeof(MASSMEDIA_AUDIO)) return false;
  const MASSMEDIA_AUDIO* pAudio = reinterpret_cast<const MASSMEDIA_AUDIO*>(data.data());

  if (pAudio->channels > 2) return false;
  if (pAudio->bits != 8 && pAudio->bits != 16 && pAudio->bits != 24 && pAudio->bits != 32) return false;
  if (pAudio->unknown != 0) return false;

  std::uint32_t dataSize = bswap_if(pAudio->dataSize);
  std::uint16_t sampleRate = bswap_if(pAudio->sampleRate);

  if (dataSize + sizeof(MASSMEDIA_AUDIO) != data.size()) return false;
  if (sampleRate < 8000 || sampleRate > 44100) return false;
  return true;
}

std::string guess_extension(const std::vector<std::byte>& data) {
  if (data.size() != 0) {
    if (is_wav_file(data)) return ".wav";
    if (is_fnt_file(data)) return ".fnt";
    if (is_chk_file(data)) return ".chk";
    if (is_img_file(data)) return ".unkimg";
    if (is_pal_file(data)) return ".unkpal";
    if (is_audio_file(data)) return ".unkpcm";
    if (is_tbl_file(data)) return ".tbl";
    if (is_grp_file(data)) return ".grp";
    if (is_txt_file(data)) return ".txt";
  }
  return ".unk";
}
