# BitGrid formats

Both formats render bytes using printable ASCII and use the same border characters. `encode` and `decode` use [HABG](#legacy-habg). `protect` and `recover` use [BGR1](#recovery-format-bgr1).

## Legacy HABG

A legacy BitGrid is an ASCII rendering of a binary header and payload. A byte is eight bits. The grid contains one record and has no version or byte-order mark.

### Binary record

| Byte offsets | Size | Field |
| --- | ---: | --- |
| 0–3 | 4 bytes | Signature `HABG`, hex `48 41 42 47` |
| 4–5 | 2 bytes | Unsigned payload length `n`, big-endian |
| 6–7 | 2 bytes | Unsigned checksum, big-endian |
| 8 through `7 + n` | `n` bytes | Payload bytes in their original order |

The length and checksum may be zero. A zero-length record ends after the checksum. Bytes are rendered most significant bit first. The payload may contain any byte value.

### Rendering

Let `b = 64 + 8n`. The interior side `d` is the smallest positive integer for which `d² >= b`. Place the bits left to right and top to bottom, starting at the upper-left interior cell. Encode each zero as `..` and each one as `##`. Fill every unused interior cell with `##`, including any padding that extends across rows.

- The top and bottom rows each contain `+`, exactly `2d` hyphens, and `+`.
- Each of the `d` interior rows contains `|`, exactly `d` bit blocks, and `|`.
- Each physical row has `2d + 2` ASCII characters before its line ending. The full grid has `d + 2` rows.

The encoder emits LF (hex `0A`) after every row. The decoder also accepts CRLF (hex `0D 0A`) throughout. The bottom row may end at EOF. Mixed separators, lone CR, extra blank lines, and whitespace outside the specified characters are rejected.

| Payload | Interior side | Physical rows | Row width | Total bytes with final LF | Total bytes with final CRLF |
| --- | ---: | ---: | ---: | ---: | ---: |
| 0 bytes | 8 | 10 | 18 | 190 | 200 |
| 65,535 bytes | 725 | 727 | 1,452 | 1,056,331 | 1,057,058 |

### Validation and compatibility

The decoder bounds the input size before parsing. The structural checks require a square interior, consistent row widths and separators, valid borders, and two matching characters in every bit block. The first 64 interior cells must contain a complete header with the `HABG` signature. The declared bytes must fit in the interior.

The canonical checks require `(d - 1)² < 64 + 8n <= d²` and all padding cells to be ones. Grid capacity can exceed the number of record bits because of the square shape. The remaining capacity is exactly the padding range. A checksum match is required before the decoder returns any payload.

Previously emitted grids still decode. Earlier decoders could accept a larger square or `..` padding. Such grids now fail the canonical checks. The earlier Windows CLI could normalize mixed separators in text mode. The current separator rules apply across platforms.

### Checksum

For unsigned payload bytes `x[1]` through `x[n]`, the stored value is

```text
checksum = (sum of p * x[p] for p = 1 through n) mod 65536
```

For example, `Hi` has bytes `48 69` in hex. Its checksum is `72 × 1 + 105 × 2 = 282` (`011A` hex). The checksum covers the payload. The signature, length, grid structure, and padding have their own validation rules.

A single-byte change by `delta` at one-based position `p` goes undetected when `p * delta` is divisible by 65,536. Equivalently, `delta` is a multiple of `65536 / gcd(p, 65536)`. Every single-byte change at an odd position is detected. At position 512 a change from `00` to `80` goes undetected. At position 32,768 any even-valued change goes undetected, while position 65,535 detects every single-byte change. Multiple changes can also cancel. The two-byte payloads `02 00` and `00 01` both have checksum 2.

A CRC-16 using the CCITT polynomial would detect all single-bit errors and all bursts spanning up to 16 bits. Its guarantees for other error patterns depend on the polynomial and record length. Changing the checksum under the existing signature would change how existing grids are interpreted. Any future CRC format would need a distinct identifier and continued legacy decoding. BitGrid retains the existing checksum for compatibility and provides neither authentication nor tamper resistance. [CRC polynomial analysis](https://users.ece.cmu.edu/~koopman/roses/dsn04/koopman04_crc_poly_embedded.pdf)

## Recovery format BGR1

BGR1 maps bytes into 12-row bands with Reed–Solomon erasure protection. The `BGR1` signature identifies the format and its version as part of the protected record. A byte is eight bits. All multi-byte integers are unsigned and big-endian.

### Protected record

| Byte offsets | Size | Field |
| --- | ---: | --- |
| 0–3 | 4 bytes | ASCII `BGR1`, hex `42 47 52 31` |
| 4–5 | 2 bytes | Payload length `n`, in bytes |
| 6 through `5 + n` | `n` bytes | Payload in its original order |
| `6 + n` through `9 + n` | 4 bytes | CRC-32C over the first `6 + n` bytes |

CRC-32C uses polynomial `0x1EDC6F41` (`0x82F63B78` reflected), with reflected input and output, initial register `0xFFFFFFFF`, and final XOR `0xFFFFFFFF`. The conventional check value for `123456789` is `0xE3069283`. For `Hi` the complete record is `42 47 52 31 00 02 48 69 EE 05 5B 24`. The CRC covers the header and payload. The decoder also verifies the zero padding and consistency of retained code symbols. [CRC-32C definition and test vectors](https://www.rfc-editor.org/rfc/rfc3720#appendix-B.4)

### Geometry and field arithmetic

Let `W = ceil((10 + n) / 8)`, `C = min(W, 32)`, and `B = ceil(W / C)`. There are `C` byte columns per band and `B` bands. Pad the record with zero bytes to `8CB` bytes. Fill the first eight rows of each band in row-major order from this buffer, then fill the four parity rows:

```text
             one codeword per byte column
           col 0   col 1       col C-1
 row  1      D       D     ...    D
   ...       .       .            .
 row  8      D       D     ...    D
 row  9      P       P     ...    P
   ...       .       .            .
 row 12      P       P     ...    P
```

In each band and column, treat the eight data bytes as the evaluations at points `0x01` through `0x08` of the unique polynomial of degree at most seven over GF(256). The four parity bytes are evaluations at points `0x09` through `0x0C`. An element's bit at index `i` is the coefficient of `x^i`. Addition and subtraction both use XOR. Multiplication is polynomial multiplication reduced modulo `x^8 + x^4 + x^3 + x^2 + 1` (`0x11D`). The code is systematic, so its first eight values are the original bytes. [Reed–Solomon codes over an erasure channel](https://www.rfc-editor.org/rfc/rfc5510#section-8)

Each byte occupies 16 ASCII characters with its most significant bit first. Render a zero bit as `..` and a one bit as `##`. Draw a `|` at each side of every band row. Top and bottom border rows contain `+`, exactly `16C` hyphens, and `+`. The physical width is `16C + 2` characters and the height is `12B + 2` rows, including borders. The writer emits LF after each row. The reader accepts uniform LF or uniform CRLF and an optional final terminator.

| Payload | Byte columns | Bands | Bytes with final LF | Bytes with final CRLF |
| --- | ---: | ---: | ---: | ---: |
| 0 bytes | 2 | 1 | 490 | 504 |
| 246 bytes | 32 | 1 | 7,210 | 7,224 |
| 65,535 bytes | 32 | 257 | 1,589,290 | 1,592,376 |

### Erasures, verification, and limits

To mark a bit's position as unknown, replace its two characters with `??`. Any `??` within a byte erases the whole symbol. All other pairs must still be well-formed. Preserve the positions of damaged cells, the border, and the row separators. A missing row can be represented by a row of `??` pairs if its position is known.

The decoder bounds the input and derives `C` and `B` from its geometry before interpreting the record. For each codeword it reconstructs the polynomial from the first eight retained symbols, checks every additional retained symbol against that polynomial, and recovers the first eight values. It then validates the signature, declared length, canonical geometry, zero padding, and CRC before returning payload bytes.

If the retained values are correct, any four erasures per band column are recoverable, including erasures in parity and record metadata. Any five erasures in one band column cause failure, even if all other columns are complete. The decoder handles known erasures. Unmarked substitutions can trigger a parity or CRC failure. In particular, when exactly eight symbols survive in a column, any eight values define a polynomial and the CRC provides a separate record-level check. CRC-32C is accidental-corruption detection and provides no authentication.
