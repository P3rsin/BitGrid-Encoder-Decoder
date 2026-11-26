# BitGrid

BitGrid is a C++17 tool for recovering bytes from partially damaged ASCII grids. Its BGR1 format uses Reed–Solomon parity down columns to reconstruct any four masked rows in each 12-row band, including rows that hold the format header, when the retained symbols are correct. The original HABG format is available through `encode` and `decode`.

## Reproduce the recovery experiment

After building, run the experiment with Python 3 and its standard library.

```sh
python3 tools/recovery_experiment.py build/bitgrid --output-dir build/demo
build/bitgrid recover build/demo/four-rows-erased.bgr > build/demo/restored.bin
cmp build/demo/original.bin build/demo/restored.bin
```

The experiment compares the same Reed–Solomon code and redundancy under two layouts. The real decoder restores all **42 placements of one to four consecutive masked rows**. A model of the same code with contiguous words survives none of those full-row losses. All eight placements of five consecutive rows fail. The script also checks six vectors against an independent encoder.

One `??` pair marks an unknown bit and erases its byte from the codeword. Framing and cell positions must be intact. Five erasures in one band column exceed the budget even if other columns are undamaged. For the 246-byte test payload, BGR1 takes 7,210 ASCII bytes. See the [case study, tradeoffs, and roadmap](DESIGN.md).

## Build and install

Use CMake 3.16 or newer and a C++17 compiler. The CI workflow covers GCC and Clang on Linux, AppleClang on macOS, and MSVC on Windows. BitGrid and its tests require no third-party libraries or test framework.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cmake --install build --config Release --prefix ./build/install
```

With a single-configuration generator the executable is `build/bitgrid`. With Visual Studio it is `build/Release/bitgrid.exe`. The example install places the executable in `build/install/bin`. Use the appropriate executable path in the examples below if `bitgrid` is outside your `PATH`.

## Usage

```text
bitgrid encode <text>
bitgrid encode -- <text>
bitgrid encode -f <file>
bitgrid encode -
bitgrid decode <file>
bitgrid decode -- <file>
bitgrid decode -
bitgrid protect <text>
bitgrid protect -- <text>
bitgrid protect -f <file>
bitgrid protect -
bitgrid recover <file>
bitgrid recover -- <file>
bitgrid recover -
bitgrid help
bitgrid --help
bitgrid --version
```

Both formats accept up to 65,535 payload bytes. Quote literal text and paths that contain spaces. Use `encode -- -` or `protect -- -f` to pass reserved arguments literally. Use `decode -- -` or `recover -- -` to open a file named `-`.

```sh
bitgrid encode "Hello world"
bitgrid encode -f "input file.bin" > data.bgrid
bitgrid decode data.bgrid > restored.bin
bitgrid protect -f "input file.bin" > protected.bgr
bitgrid recover protected.bgr > restored.bin
printf 'first\nsecond' | bitgrid encode - | bitgrid decode -
bitgrid decode - < data.bgrid > restored.bin
```

File and standard-stream I/O preserve bytes, including NUL, CRLF sequences in the payload, and UTF-8. On Windows, use byte-preserving redirection or pipelines, such as those in `cmd.exe`, for binary data. Literal arguments and file names on Windows use the narrow C runtime encoding. Use a file or standard input to supply arbitrary payload bytes.

Successful output goes to stdout and diagnostics go to stderr. Exit status is `0` on success and `1` on usage, input, validation, or output failure. Decoding writes the payload exactly as stored, with no added newline. An I/O failure during a write can leave partial output.

## Legacy HABG example

```sh
bitgrid encode "Hi" > hi.bgrid
bitgrid decode hi.bgrid
```

The encoder writes the following grid to `hi.bgrid`. The decoder emits the two bytes `Hi`.

```text
+------------------+
|..##....##........|
|##..........##..##|
|........##....##..|
|....######........|
|..................|
|..##..............|
|..##......####..##|
|....##....##......|
|..####..##....####|
+------------------+
```

## Format and validation

Each logical bit occupies two printable characters. `..` represents zero and `##` represents one. A 64-bit header stores the ASCII signature `HABG`, the payload length, and the checksum. The header and payload fill the smallest square that can hold them, with `##` in all unused cells.

Both readers check dimensions, borders, cells, payload capacity, and canonical padding. They accept uniform LF or CRLF with an optional final terminator. BGR1 reconstructs the record before checking its CRC-32C. It handles marked erasures and can detect unmarked substitutions through parity and CRC checks. See the [format specification](FORMAT.md) for byte ordering, bounds, field arithmetic, and the legacy checksum's limitations.

## Development

`BitGridCodec.h` exposes stateless functions in an internal static library. They accept borrowed `std::string_view` input and return an owned string or a typed `CodecError`. `BitGridRecovery.cpp` implements finite-field interpolation, interleaving, and record validation. The legacy codec and recovery codec share grid framing. `main.cpp` handles command syntax, bounded I/O, and diagnostics.

```sh
cmake -E chdir build ctest -C Release --output-on-failure

cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=Debug -DBITGRID_ENABLE_SANITIZERS=ON
cmake --build build-sanitize --config Debug
cmake -E chdir build-sanitize ctest -C Debug --output-on-failure
```

The sanitizer option enables AddressSanitizer and UndefinedBehaviorSanitizer with GCC or Clang where their runtimes are available. Configure with `-DBUILD_TESTING=OFF` to build only the utility and its codec library. CTest exercises all 4,096 row masks on a known-answer grid, maximum-size binary recovery, malformed framing, parity and CRC failures, and CLI file and stream I/O.

Apply the repository's `.clang-format` style to C++ changes.

## Author

Haroon Awan · [GitHub](https://github.com/P3rsin)
