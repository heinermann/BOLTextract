## Overview
[Mass Media Games](https://en.wikipedia.org/wiki/Mass_Media_Games)' BOLT archive extractor. Includes [Mass Media Games, Inc](https://www.mobygames.com/company/1562/mass-media-games-inc/) and [Philips P.O.V. Entertainment Group](https://www.mobygames.com/company/7588/philips-pov-entertainment-group/).

Unpacks all files contained within the embedded archive. The files are not named. Some enclosed file formats are common between games. Tested on various different roms. Failure modes are not tested.

## Games that use BOLT
Here are some games confirmed to use BOLT archives:
- Mystic Midway: Rest in Pieces (DOS)
- Voyeur (DOS)
- 3-D TableSports (DOS)
- The Game of Life (Windows 95)
- Bassmasters 2000 (N64)
- Namco Museum (N64, GBA, Dreamcast, PS2, XBox)
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
  -a, --algo cdi|dos|n64|gba|win|xbox|ps2
                                Choose algorithm to use. (default: "")
  -h, --help                    show help
```

Example: `bolt-extract.exe -a n64 -b "StarCraft 64 (U).z64" starcraft64/`

## Supported Algorithms
- `cdi` - For some older CD-i games before 1993.
- `dos` - Either from MSDOS or CD-i games between 1993 and 1996.
- `win` - For The Game of Life (1998).
- `n64`/`gba` - Used in games released between 1999 and 2003. You may need to manually separate BOLT archives in gba roms.
- `xbox`/`ps2` - Same as the n64 algorithm but with altered data structures. Used in games released from 2004 onward.

## Notes
- Only `z64` format roms for N64 are supported.
- Not all CD-i game archives are supported.
- Other consoles and newer games untested.

## Building
1. `git clone https://github.com/heinermann/BOLTextract.git`
2. `git submodule init`
3. Install Visual Studio 2019
4. Double click the `.sln` file and build

