# BitGrid format

A BitGrid is an ASCII rendering of a binary header and payload. A byte is eight bits. The grid contains one record and has no version or byte-order mark.

## Binary record

| Byte offsets | Size | Field |
| --- | ---: | --- |
| 0–3 | 4 bytes | Signature `HABG`, hex `48 41 42 47` |
| 4–5 | 2 bytes | Unsigned payload length `n`, big-endian |
| 6–7 | 2 bytes | Unsigned checksum, big-endian |
| 8 through `7 + n` | `n` bytes | Payload bytes in their original order |

The length and checksum may be zero. A zero-length record ends after the checksum. Bytes are rendered most significant bit first. The payload may contain any byte value.

## Rendering

Let `b = 64 + 8n`. The interior side `d` is the smallest positive integer for which `d² >= b`. Place the bits left to right and top to bottom, starting at the upper-left interior cell. Encode each zero as `..` and each one as `##`. Fill every unused interior cell with `##`, including any padding that extends across rows.

- The top and bottom rows each contain `+`, exactly `2d` hyphens, and `+`.
- Each of the `d` interior rows contains `|`, exactly `d` bit blocks, and `|`.
- Each physical row has `2d + 2` ASCII characters before its line ending. The full grid has `d + 2` rows.

The encoder emits LF (hex `0A`) after every row. The decoder also accepts CRLF (hex `0D 0A`) throughout. The bottom row may end at EOF. Mixed separators, lone CR, extra blank lines, and whitespace outside the specified characters are rejected.

| Payload | Interior side | Physical rows | Row width | Total bytes with final LF | Total bytes with final CRLF |
| --- | ---: | ---: | ---: | ---: | ---: |
| 0 bytes | 8 | 10 | 18 | 190 | 200 |
| 65,535 bytes | 725 | 727 | 1,452 | 1,056,331 | 1,057,058 |

## Validation and compatibility

The decoder bounds the input size before parsing. The structural checks require a square interior, consistent row widths and separators, valid borders, and two matching characters in every bit block. The first 64 interior cells must contain a complete header with the `HABG` signature. The declared bytes must fit in the interior.

The canonical checks require `(d - 1)² < 64 + 8n <= d²` and all padding cells to be ones. Grid capacity can exceed the number of record bits because of the square shape. The remaining capacity is exactly the padding range. A checksum match is required before the decoder returns any payload.

Previously emitted grids still decode. Earlier decoders could accept a larger square or `..` padding. Such grids now fail the canonical checks. The earlier Windows CLI could normalize mixed separators in text mode. The current separator rules apply across platforms.

## Checksum

For unsigned payload bytes `x[1]` through `x[n]`, the stored value is

```text
checksum = (sum of p * x[p] for p = 1 through n) mod 65536
```

For example, `Hi` has bytes `48 69` in hex. Its checksum is `72 × 1 + 105 × 2 = 282` (`011A` hex). The checksum covers the payload. The signature, length, grid structure, and padding have their own validation rules.

A single-byte change by `delta` at one-based position `p` goes undetected when `p * delta` is divisible by 65,536. Equivalently, `delta` is a multiple of `65536 / gcd(p, 65536)`. Every single-byte change at an odd position is detected. At position 512 a change from `00` to `80` goes undetected. At position 32,768 any even-valued change goes undetected, while position 65,535 detects every single-byte change. Multiple changes can also cancel. The two-byte payloads `02 00` and `00 01` both have checksum 2.

A CRC-16 using the CCITT polynomial would detect all single-bit errors and all bursts spanning up to 16 bits. Its guarantees for other error patterns depend on the polynomial and record length. Changing the checksum under the existing signature would change how existing grids are interpreted. Any future CRC format would need a distinct identifier and continued legacy decoding. BitGrid retains the existing checksum for compatibility and provides neither authentication nor tamper resistance. [CRC polynomial analysis](https://users.ece.cmu.edu/~koopman/roses/dsn04/koopman04_crc_poly_embedded.pdf)
