## Overview
Starcraft 64 BOLT archive extractor.

Unpacks all files contained within the embedded archive (maps, images, script, encyclopedia files, etc).

It does **NOT** support listfiles, since the hash mechanism is unknown. Therefore files will not have names, but it will try to guess an extension so you know generally what the file is.

Tested on various different versions. Failure modes are not tested.

## Running

```
Extract BOLT archive from a Starcraft z64 rom.
Usage:
  bolt-extract [OPTION...] INPUT_FILE OUTPUT_DIR

  -h, --help  show help
```

Example: `bolt-extract.exe "StarCraft 64 (U).z64" starcraft64/`

## Building
Install Visual Studio 2019, double click the `.sln` file, build.

