# CHDtoOGG
A tool to convert a CHD CD image file to OGG/BIN/CUE files.

Unlike most OGG encoders, the output of CHDtoOGG is standardized and bit-exact regardless of hardware or system used to run the encoder.

## Download
You can find a download for Windows under the [Releases page](../../releases/latest).  
For other platforms, check the section on [compiling](#compiling).

## Usage
The tool is used in the command prompt with input and output file required:
```sh
CHDtoOGG v1.0 - Command line options:
  -i <PATH>  : Path to input CHD file (required)
  -o <PATH>  : Path to output CUE file (required)
  -q <LEVEL> : Quality level 0 to 10, defaults to 8
  -n         : Output an empty data track
  -x         : Print XML DAT metadata
```

Example:  
`CHDtoOGG -i "Game (USA).chd" -o "Game (USA).cue"`

### Path to input CHD file (required)
The `-i path.chd` option must be set and specify an uncompressed version 5 CHD file (created with `chdman createcd -c none`) with CD tracks that should be converted to CUE/BIN/OGG.

### Path to output CUE file (required)
The `-o path.cue` option must be set and specify the name of the .cue file to be generated. The CD tracks in the CHD file are then output next to the .cue file in the following format:
- path.cue - The cue file which acts as the index file for the CD tracks
- path (Track 1).bin - The data track (which can either be from the source or [empty](#output-an-empty-data-track)
- path (Track N).ogg - The audio tracks in OGG Vorbis format

### Quality level
The optional `-q LEVEL` option can specify a different quality level than the default level of 8.

|   Quality Level  | Nominal Bitrate |
|------------------|-----------------|
| `-q 0`           |   64 kbit/s     |
| `-q 1`           |   80 kbit/s     |
| `-q 2`           |   96 kbit/s     |
| `-q 3`           |  112 kbit/s     |
| `-q 4`           |  128 kbit/s     |
| `-q 5`           |  160 kbit/s     |
| `-q 6`           |  192 kbit/s     |
| `-q 7`           |  224 kbit/s     |
| `-q 8` (default) |  256 kbit/s     |
| `-q 9`           |  320 kbit/s     |
| `-q 10`          |  500 kbit/s     |

### Output an empty data track
If specifying the optional `-n` option, the files on the original data track will be discarded and just a tiny, empty .BIN file will be output.
This can be used to keep the track layout of the original CD when only the audio tracks are desired.

### Print XML DAT metadata
If specifying the optional `-x` option, the program will output XML DAT metadata to be contributed to the DAT project.

## Compiling
On Windows open the Visual Studio solution and press build.  
For other platforms use either `./build-gcc.sh` or `./build-clang.sh` to compile the tool for your system.

## Determinism
To make the OGG Vorbis encoding deterministic across platforms and systems, this project uses a precompiled version of the OGG Vorbis
encoder by Xiph which has been compiled to webassembly and reverse transpiled back to C. The final processed version is [EncodeVorbis.wasm.cpp](EncodeVorbis.wasm.cpp).
You can find the original source code of the encoder in the [EncodeVorbis](EncodeVorbis) directory. Although a new version of the encoder should not be generated
unless breaking of compatibility with all outputs generated so far is acceptable. Using different versions of compilers or tools, or different methods to generate
the code also can lead to breaking of compatibility because different kinds of code optimizations can lead to different results in the encoding process.

## License
The project is distributed under the 3-Clause BSD License, same as [libogg](https://www.xiph.org/ogg/) and [libvorbis](https://xiph.org/vorbis/).
