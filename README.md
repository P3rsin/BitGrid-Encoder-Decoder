# BitGrid

BitGrid is a C++17 command-line utility that encodes bytes as a bordered ASCII grid and decodes them back to their original byte values. It accepts literal arguments, files, and standard input. The format supports up to 65,535 payload bytes and includes a 16-bit checksum for accidental corruption detection.

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
bitgrid help
bitgrid --help
bitgrid --version
```

Quote literal text and paths that contain spaces. Use `encode -- -` or `encode -- -f` to encode those arguments literally. Use `decode -- -` to open a file named `-`.

```sh
bitgrid encode "Hello world"
bitgrid encode -f "input file.bin" > data.bgrid
bitgrid decode data.bgrid > restored.bin
printf 'first\nsecond' | bitgrid encode - | bitgrid decode -
bitgrid decode - < data.bgrid > restored.bin
```

File and standard-stream I/O preserve bytes, including NUL, CRLF sequences in the payload, and UTF-8. On Windows, use byte-preserving redirection or pipelines, such as those in `cmd.exe`, for binary data. Literal arguments and file names on Windows use the narrow C runtime encoding. Use a file or standard input to supply arbitrary payload bytes.

Successful output goes to stdout and diagnostics go to stderr. Exit status is `0` on success and `1` on usage, input, validation, or output failure. Decoding writes the payload exactly as stored, with no added newline. An I/O failure during a write can leave partial output.

## Example

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

The decoder checks the size, row layout, borders, cells, complete header, signature, payload capacity, canonical dimensions, padding, and checksum. It accepts uniform LF or CRLF, with or without the final row terminator. A grid occupies up to 1,057,058 bytes including its borders and line endings. Even a single changed payload byte can go undetected at some positions. See [the format specification](FORMAT.md) for the wire layout and checksum analysis.

## Development

`BitGridCodec.h` exposes stateless encode and decode functions in an internal static library. They accept borrowed `std::string_view` input and return an owned string or a typed `CodecError` in a `std::variant`. `main.cpp` handles command syntax, bounded I/O, and diagnostics.

```sh
cmake -E chdir build ctest -C Release --output-on-failure

cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=Debug -DBITGRID_ENABLE_SANITIZERS=ON
cmake --build build-sanitize --config Debug
cmake -E chdir build-sanitize ctest -C Debug --output-on-failure
```

The sanitizer option enables AddressSanitizer and UndefinedBehaviorSanitizer with GCC or Clang where their runtimes are available. Configure with `-DBUILD_TESTING=OFF` to build only the utility and its codec library. The codec tests include independent known-answer grids and deterministic round trips and mutations. CLI tests use CMake to run the executable with files, redirected streams, and a real pipe.

Apply the repository's `.clang-format` style to C++ changes.

## Author

Haroon Awan · [GitHub](https://github.com/P3rsin)
