# BitGrid

**BitGrid** is a small C++17 command-line utility that encodes byte-oriented input into a custom two-dimensional binary format and decodes it with structural validation, metadata checks, and checksum verification.

The project is intentionally small, but is designed as a complete CLI utility rather than an interactive menu program: it supports literal arguments, files, standard input/output, typed codec errors, and a reproducible CMake build.

## Features

- Encode text or file contents into a custom BitGrid representation
- Decode BitGrid files or data read from standard input
- Pipe and redirect data through standard Unix-style streams
- Store a 64-bit metadata header containing:
  - a 32-bit format signature
  - a 16-bit payload length
  - a 16-bit checksum
- Validate grid dimensions, borders, and encoded bit blocks
- Reject incomplete headers, invalid signatures, truncated payloads, and checksum mismatches
- Preserve multiline input when reading from files or standard input
- Build with CMake and the C++17 standard
- No external library dependencies

## Build

### Requirements

- CMake 3.16 or newer
- A C++17-compatible compiler such as GCC or Clang

Clone the repository:

```bash
git clone https://github.com/P3rsin/BitGrid-Encoder-Decoder.git
cd BitGrid-Encoder-Decoder
```

Configure and build:

```bash
cmake -S . -B build
cmake --build build
```

The executable will be created at:

```text
./build/bitgrid
```

To see the available commands:

```bash
./build/bitgrid help
```

## Usage

```text
bitgrid encode <text>        Encode literal text into a BitGrid
bitgrid encode -f <file>     Encode the contents of a file
bitgrid encode -             Encode data read from standard input

bitgrid decode <file>        Decode a BitGrid file
bitgrid decode -             Decode a BitGrid read from standard input

bitgrid help                 Display command help
```

When running directly from the build directory, use `./build/bitgrid` in place of `bitgrid`.

### Encode literal text

```bash
./build/bitgrid encode "Hello world"
```

Multiword text should be quoted so the shell passes it as one argument.

### Encode a file

```bash
./build/bitgrid encode -f input.txt > encoded.bgrid
```

The encoded BitGrid is written to standard output, so normal shell redirection can be used to save it.

### Encode from standard input

```bash
printf 'Hello\nworld' | ./build/bitgrid encode -
```

or:

```bash
./build/bitgrid encode - < input.txt > encoded.bgrid
```

### Decode a BitGrid file

```bash
./build/bitgrid decode encoded.bgrid
```

### Decode from standard input

```bash
./build/bitgrid decode - < encoded.bgrid
```

Because encoded output is written to `stdout` and diagnostic messages are written to `stderr`, BitGrid can be used cleanly in pipelines and shell scripts.

## Example

Encoding:

```bash
./build/bitgrid encode "Hi"
```

produces:

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

The result can be saved and decoded again:

```bash
./build/bitgrid encode "Hi" > hi.bgrid
./build/bitgrid decode hi.bgrid
```

Output:

```text
Hi
```

## BitGrid Format

BitGrid converts each input byte into eight binary bits. A 64-bit header is prepended to the payload before the complete bit sequence is rendered into the grid.

### Header

| Field | Size | Purpose |
| --- | ---: | --- |
| `HABG` signature | 32 bits | Identifies the BitGrid format |
| Payload length | 16 bits | Number of encoded input bytes |
| Checksum | 16 bits | Basic payload integrity check |

The maximum payload size is therefore **65,535 bytes**.

### Bit representation

Each logical bit cell occupies two printable characters:

| Block | Value |
| --- | ---: |
| `##` | `1` |
| `..` | `0` |

The grid border uses:

| Character | Purpose |
| --- | --- |
| `+` | Corner |
| `-` | Horizontal border |
| `\|` | Vertical border |

The header and payload bits are written row-by-row into the smallest square interior that can contain them:

```text
interior dimension = ceil(sqrt(header bits + payload bits))
```

A one-cell border is then added around the interior.

If the final row contains unused cells, those cells are padded with `1` blocks (`##`). The payload-length field tells the decoder exactly how many payload bytes belong to the encoded data.

## Checksum

BitGrid uses a simple position-weighted 16-bit checksum.

For each payload byte, the unsigned byte value is multiplied by its one-based position:

```text
checksum = Σ(byte[i] * (i + 1)) mod 65536
```

During decoding, BitGrid recalculates the checksum from the recovered payload and compares it with the value stored in the header.

The checksum is intended only to detect accidental corruption. It is **not cryptographically secure** and must not be used for authentication or tamper resistance.

## Validation

Decoding is performed in stages.

BitGrid first validates the physical grid structure:

- the input contains enough rows to form a grid;
- every row has the expected width;
- the logical row and column counts form a square;
- the top and bottom borders are correctly formed;
- each interior row has valid left and right borders;
- every interior data cell is either `..` or `##`.

It then validates the encoded data:

- the complete 64-bit header is present;
- the signature matches `HABG`;
- the payload length declared by the header is available;
- the decoded payload matches the stored checksum.

Malformed input is reported on `stderr`, and the program exits with a nonzero status.

## Design

The program is split into two main responsibilities:

```text
CLI / main.cpp
    |
    |  arguments, files, stdin/stdout, error reporting
    v
BitGridCodec
    |
    |  encoding, parsing, validation, checksum
    v
encoded or decoded data
```

`BitGridCodec` does not retain the previously encoded or decoded payload as object state. Each encode/decode operation receives its input directly and returns either a result or a `CodecError` through `std::variant`.

Low-level binary conversion helpers are kept internal to the codec implementation.

## Project Structure

```text
.
├── BitGridCodec.cpp
├── BitGridCodec.h
├── CMakeLists.txt
├── main.cpp
└── README.md
```

## Limitations

- Payload size is limited to **65,535 bytes** by the 16-bit length field.
- The checksum provides basic error detection only.
- Input is byte-oriented rather than Unicode-code-point-aware.
- Padding cells are generated as `1` blocks, but padding contents are not currently validated during decoding.
- BitGrid is a custom format and is not compatible with QR codes or other standardized barcode formats.

## Author

**Haroon Awan**

[GitHub](https://github.com/P3rsin)
