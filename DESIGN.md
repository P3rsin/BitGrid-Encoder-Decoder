# BitGrid design case study

BitGrid is an experimental encoding system for studying how a text layout affects data recovery. HABG renders a byte record as a square of ASCII pairs. Its weighted checksum can miss changes, including a change from `00` to `80` at byte position 512, and provides no way to reconstruct a missing byte. BGR1 adds a separate recovery format so that the original commands and valid HABG grids continue to work. [Wire specification](FORMAT.md)

## Why Reed–Solomon

BGR1 uses a systematic Reed–Solomon (12,8) code. In a column, the eight data bytes are values of a polynomial of degree at most seven at field points 1 through 8. Four parity bytes are its values at points 9 through 12. Systematic means the data bytes themselves appear in the codeword.

GF(256) represents each byte as a polynomial with binary coefficients. XOR adds and subtracts those polynomials. Carryless multiplication is reduced modulo `0x11D`. The independent check verifies that successive powers of the residue `x` visit every nonzero byte and return to 1. All nonzero residues are therefore units, so the modulus is irreducible and the quotient is a field. The C++ inverse table uses `a^254` for the nonzero differences between evaluation points. [RFC 5510 field and code construction](https://www.rfc-editor.org/rfc/rfc5510#section-8)

Any eight distinct points uniquely determine the polynomial. The C++ decoder forms Newton divided differences from the first eight retained points and evaluates the resulting polynomial in nested form. A nonzero polynomial of degree at most seven can have at most seven roots, so distinct codewords differ in at least five symbols. A polynomial with seven chosen roots attains that bound. This gives a minimum distance of five and enough surviving points to recover any four known erasures. With `s` erasures and between one and `4 - s` incorrect retained symbols, the decoder will reject the inconsistent word. It uses `??` as the sole erasure marker.

## Layout and integrity

Each band stacks eight data rows and four parity rows. A codeword runs vertically within one byte column. A full missing row therefore removes one symbol from each codeword in the band. Four full rows use the four-erasure allowance in every column. A narrow vertical cut can use up the allowance in one column while leaving the rest of the band intact. The [format diagram](FORMAT.md#geometry-and-field-arithmetic) shows this mapping.

Framing lets the decoder bound and index the grid before reading any metadata. The signature, payload length, CRC, and payload are all part of the protected record. After reconstruction, the decoder checks geometry, zero padding, and CRC-32C. The CRC covers the signature, length, and payload. With exactly eight retained symbols in a column, every set of values determines some polynomial, so the CRC can reject corruption that parity consistency cannot distinguish. CRC-32C detects accidental corruption and supplies no authenticity guarantee. [CRC-32C definition](https://www.rfc-editor.org/rfc/rfc3720#appendix-B.4)

## Measured tradeoff

The [experiment](tools/recovery_experiment.py) exercises every horizontal placement in one full, 32-column band. It runs the actual interleaved decoder and compares its result with the theoretical erasure counts of the same code laid out in contiguous 12-byte words. A full 32-byte row in the contiguous arrangement contains at least one complete word.

| Consecutive rows masked | Placements | Interleaved recoveries | Contiguous survivors in the model |
| ---: | ---: | ---: | ---: |
| 1 | 12 | 12 | 0 |
| 2 | 11 | 11 | 0 |
| 3 | 10 | 10 | 0 |
| 4 | 9 | 9 | 0 |
| 5 | 8 | 0 | 0 |

Parity adds four symbols per eight data symbols, or 50%. Framing, band rounding, and ASCII pairs add further overhead. A 246-byte payload takes 7,210 ASCII bytes in BGR1 and 4,560 bytes in HABG. A maximum-size BGR1 record occupies 1,589,290 bytes with LF, roughly 24.25 times its payload size.

The code uses 28 divided differences and four seven-step evaluations per codeword. Grid rendering, parsing, and CRC processing therefore take linear time in grid size with the code parameters fixed. The codec keeps one padded data record of at most 65,792 bytes, plus fixed-size interpolation scratch. This excludes caller-owned input and the returned grid or payload. The CLI buffers one bounded input before invoking the codec.

## Verification and scope

The reference encoder uses product-then-reduce field arithmetic, Gauss–Jordan elimination on a Vandermonde system, and an MSB-first CRC with explicit reflection. The C++ path uses shift-and-reduce arithmetic, Newton interpolation, and the reflected CRC polynomial. Byte-exact comparisons cover seven grids, including 64 binary basis vectors for the linear parity map. CTest also exercises all 4,096 row masks on an independent known-answer grid, mixed column masks across bands, and maximum-size recovery.

A CRC alone would improve detection while leaving erased bytes unavailable. One XOR parity row would handle a single erasure per column. Support for correcting unknown error locations would require a different decoding algorithm and independent tests for its correction bound. The release focuses on marked damage in aligned ASCII. Camera scanning, archival interchange, and production communications are outside that experimental scope.

The recovery milestone was developed with AI assistance. The source, reference encoder, and test vectors are available for independent review.
