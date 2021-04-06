## Overview
[Mass Media Games](https://en.wikipedia.org/wiki/Mass_Media_Games)' BOLT archive extractor.

Unpacks all files contained within the embedded archive. The files are not named. Some enclosed file formats are common between games. Tested on various different roms. Failure modes are not tested.

## Games that use BOLT
Here are some games confirmed to use BOLT archives:
- **Starcraft 64** (N64)
- Namco Museum (N64, GBA, Dreamcast)
- Bassmasters 2000 (N64)
- Ms. Pac-Man - Maze Madness (N64, Dreamcast)
- Power Rangers - Lightspeed Rescue (N64)
- Shrek Super Party (XBox)
- Pac-Man Collection (GBA)
- The Lost Vikings (GBA)
- Rock n' Roll Racing (GBA)
- Blackthorne (GBA)
- The Game of Life (Windows 95)

## Running
```
Extract Mass Media's BOLT archive from binaries.
Usage:
  bolt-extract [OPTION...] INPUT_FILE [OUTPUT_DIR]

  -b, --big   Use Big Endian byte order (N64)
  -h, --help  show help
```

Example: `bolt-extract.exe -b "StarCraft 64 (U).z64" starcraft64/`

## Building
1. `git clone https://github.com/heinermann/BOLTextract.git`
2. `git submodule init`
3. Install Visual Studio 2019
4. Double click the `.sln` file and build

