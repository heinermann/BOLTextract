## Overview
[Mass Media Games](https://en.wikipedia.org/wiki/Mass_Media_Games)' BOLT archive extractor.

Unpacks all files contained within the embedded archive. The files are not named. Some enclosed file formats are common between games. Tested on various different roms. Failure modes are not tested.

## Games that use BOLT
Here are some games confirmed to use BOLT archives:
- Mystic Midway: Rest in Pieces (DOS)
- Voyeur (DOS)
- 3-D TableSports (DOS)
- The Game of Life (Windows 95)
- Bassmasters 2000 (N64)
- Namco Museum (N64, GBA, Dreamcast)
- Ms. Pac-Man - Maze Madness (N64, Dreamcast)
- Power Rangers - Lightspeed Rescue (N64)
- **Starcraft 64** (N64)
- Pac-Man Collection (GBA)
- Shrek Super Party (XBox)
- Blackthorne (GBA)
- Rock n' Roll Racing (GBA)
- The Lost Vikings (GBA)


## Running
```
Extract Mass Media's BOLT archive from binaries.
Usage:
  bolt-extract [OPTION...] INPUT_FILE [OUTPUT_DIR]

  -b, --big                     Use Big Endian byte order (N64, CD-i)
  -a, --algo cdi|dos|n64|gba|win
                                Choose algorithm to use. (default: "")
  -h, --help                    show help
```

Example: `bolt-extract.exe -a n64 -b "StarCraft 64 (U).z64" starcraft64/`

## Supported Algorithms
- `n64` - Archive in z64 rom.
- `gba` - Archive gba rom.
- `dos` - Archive either from MSDOS game or CD-i game.
- `cdi` - Archive for some older CD-i games.
- `win` - Archive for Windows game.

## Notes
- Only `z64` format roms for N64 are supported.
- Not all CD-i game archives are supported.
- Other consoles and newer games untested.

## Building
1. `git clone https://github.com/heinermann/BOLTextract.git`
2. `git submodule init`
3. Install Visual Studio 2019
4. Double click the `.sln` file and build

