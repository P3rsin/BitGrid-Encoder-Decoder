#!/usr/bin/env python3
"""Generate independent BGR1 vectors and compare two layouts under row damage."""

import argparse
import functools
from html import escape
import operator
from pathlib import Path
import subprocess
from textwrap import wrap


def gf_multiply(a, b):
    # Form the full carryless product, then reduce by polynomial long division.
    product = 0
    while b:
        if b & 1:
            product ^= a
        a <<= 1
        b >>= 1
    while product.bit_length() > 8:
        product ^= 0x11D << (product.bit_length() - 9)
    return product


@functools.lru_cache(maxsize=None)
def gf_inverse(value):
    return next(b for b in range(1, 256) if gf_multiply(value, b) == 1)


def powers(point):
    values = [1]
    for _ in range(7):
        values.append(gf_multiply(values[-1], point))
    return values


def parity_weights(point):
    # Solve V^T * weights = [1, point, ..., point^7] via Gauss-Jordan
    # elimination. The C++ codec instead uses Newton interpolation.
    columns = [powers(i) for i in range(1, 9)] + [powers(point)]
    matrix = [list(row) for row in zip(*columns)]
    for col in range(8):
        pivot = next(row for row in range(col, 8) if matrix[row][col])
        matrix[col], matrix[pivot] = matrix[pivot], matrix[col]
        inverse = gf_inverse(matrix[col][col])
        matrix[col] = [gf_multiply(inverse, x) for x in matrix[col]]
        for row in range(8):
            if row != col:
                factor = matrix[row][col]
                matrix[row] = [
                    a ^ gf_multiply(factor, b) for a, b in zip(matrix[row], matrix[col])
                ]
    return [row[-1] for row in matrix]


def crc32c(data):
    def reflect(value, width):
        return int(f"{value:0{width}b}"[::-1], 2)

    # Use the normal polynomial and explicit reflection for the reference.
    crc = 0xFFFFFFFF
    for byte in data:
        crc ^= reflect(byte, 8) << 24
        for _ in range(8):
            polynomial = 0x1EDC6F41 if crc & 0x80000000 else 0
            crc = ((crc << 1) & 0xFFFFFFFF) ^ polynomial
    return reflect(crc, 32) ^ 0xFFFFFFFF


def reference_grid(payload):
    record = b"BGR1" + len(payload).to_bytes(2, "big") + payload
    record += crc32c(record).to_bytes(4, "big")
    words = (len(record) + 7) // 8
    columns = min(words, 32)
    band_capacity = 8 * columns
    weights = [parity_weights(point) for point in range(9, 13)]
    border = b"+" + b"-" * (16 * columns) + b"+"
    lines = [border]
    for start in range(0, len(record), band_capacity):
        band = record[start : start + band_capacity].ljust(band_capacity, b"\0")
        data_rows = [band[i : i + columns] for i in range(0, band_capacity, columns)]
        words = list(zip(*data_rows))
        parity_rows = [
            bytes(
                functools.reduce(operator.xor, (gf_multiply(w, x) for w, x in zip(row, word)))
                for word in words
            )
            for row in weights
        ]
        for row in data_rows + parity_rows:
            cells = "".join(f"{byte:08b}" for byte in row).replace("0", "..").replace("1", "##")
            lines.append(b"|" + cells.encode("ascii") + b"|")
    return b"\n".join([*lines, border]) + b"\n"


def erase_rows(grid, start, count):
    lines = grid.splitlines()
    for row in range(start + 1, start + count + 1):
        lines[row] = b"|" + b"?" * (len(lines[row]) - 2) + b"|"
    return b"\n".join(lines) + b"\n"


def contiguous_words_survive(start, count, columns):
    # Same code, same grid size, symbols laid out one entire word at a time.
    erased = range(start * columns, (start + count) * columns)
    losses = [0] * columns
    for position in erased:
        losses[position // 12] += 1
    return max(losses) <= 4


def run(executable, command, data):
    # A timeout makes the experiment fail if a regression stalls the parser.
    return subprocess.run(
        [str(executable), command, "-"], input=data, capture_output=True, timeout=30
    )


def require(condition, message):
    if not condition:
        raise SystemExit(message)


def verify_field():
    # If x generates all 255 nonzero residues, every one is a unit and
    # the degree-eight modulus is irreducible.
    value = 1
    residues = set()
    for _ in range(255):
        residues.add(value)
        value = gf_multiply(value, 2)
    require(value == 1 and residues == set(range(1, 256)), "GF(256) generator cycle failed")


def parity_basis_payload():
    # Bands two and three each have 32 columns. Place one of the 64 binary
    # basis vectors of the 8-symbol input in each column, away from metadata.
    payload = bytearray(3 * 256 - 6)
    for index in range(64):
        band, column = 1 + index // 32, index % 32
        row, bit = divmod(index, 8)
        payload[band * 256 + row * 32 + column - 6] = 1 << bit
    return bytes(payload)


def write_demo_svg(executable, path):
    payload = b"Hi"
    protected = run(executable, "protect", payload)
    require(protected.returncode == 0 and not protected.stderr, "demo protection failed")
    intact = protected.stdout
    four_rows = erase_rows(intact, 0, 4)
    five_rows = erase_rows(intact, 0, 5)
    restored = run(executable, "recover", four_rows)
    failed = run(executable, "recover", five_rows)
    require(restored.returncode == 0 and restored.stdout == payload and not restored.stderr,
            "demo recovery failed")
    require(failed.returncode == 1 and not failed.stdout and b"more than four" in failed.stderr,
            "demo exceeded-budget check failed")

    panels = (
        ("Intact", intact, f"protect exit {protected.returncode}", ()),
        ("Four rows erased", four_rows, f"recover exit {restored.returncode}",
         (f'stdout  "{restored.stdout.decode("ascii")}"',)),
        ("Five rows erased", five_rows, f"recover exit {failed.returncode}",
         (*wrap(failed.stderr.decode("ascii").strip(), width=34), "stdout  (empty)")),
    )
    svg = [
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 780 300" role="img" aria-labelledby="title desc">',
        '<title id="title">BitGrid BGR1 erasure recovery</title>',
        '<desc id="desc">The original Hi grid, four masked rows restored to Hi, '
        'and five masked rows rejected with empty standard output.</desc>',
        '<rect width="780" height="300" rx="10" fill="#eef2f7"/>',
    ]
    for index, (heading, grid, status, output) in enumerate(panels):
        svg += [
            f'<g transform="translate({10 + 256 * index} 10)">',
            '<rect width="248" height="280" rx="7" fill="#ffffff" stroke="#bac6d4"/>',
            f'<text x="12" y="24" fill="#172334" font-family="sans-serif" '
            f'font-weight="bold" font-size="14">{escape(heading)}</text>',
            '<g font-family="monospace" font-size="10.5" xml:space="preserve">',
        ]
        for row_number, row in enumerate(grid.decode("ascii").splitlines()):
            color = "#a12b1d" if "?" in row else "#243246"
            positions = " ".join(str(12 + 6 * column) for column in range(len(row)))
            svg.append(
                f'<text x="{positions}" y="{47 + 12 * row_number}" fill="{color}" '
                f'font-family="DejaVu Sans Mono">{escape(row)}</text>'
            )
        for line_number, line in enumerate((status, *output)):
            color = "#a12b1d" if index == 2 else "#1b664c"
            svg.append(f'<text x="12" y="{222 + 13 * line_number}" fill="{color}">{escape(line)}</text>')
        svg += ["</g>", "</g>"]
    svg.append("</svg>")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(("\n".join(svg) + "\n").encode("utf-8"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bitgrid", type=Path, help="built BitGrid executable")
    parser.add_argument("--output-dir", type=Path, help="write pristine and damaged samples")
    parser.add_argument("--demo-svg", type=Path, help="render the verified recovery and failure demo")
    args = parser.parse_args()
    executable = args.bitgrid.resolve()
    verify_field()
    print("GF(256) generator visits all 255 nonzero residues.")
    require(crc32c(b"123456789") == 0xE3069283, "CRC-32C check vector failed")

    # Exactly one full band with 32 codewords and no zero padding.
    payload = bytes(range(246))
    fixtures = (b"", b"Hi", b"\0\r\n\x1a\x80\xff", payload, bytes(range(247)), bytes(range(256)),
                parity_basis_payload())
    for sample in fixtures:
        reference = reference_grid(sample)
        encoded = run(executable, "protect", sample)
        require(
            encoded.returncode == 0 and not encoded.stderr and encoded.stdout == reference,
            f"reference encoding mismatch at {len(sample)} payload bytes",
        )
        recovered = run(executable, "recover", reference)
        require(
            recovered.returncode == 0 and not recovered.stderr and recovered.stdout == sample,
            f"reference decoding mismatch at {len(sample)} payload bytes",
        )
    print(f"Independent encoder agrees on {len(fixtures)} grids, covering all 64 parity basis vectors.")

    grid = reference_grid(payload)
    legacy = run(executable, "encode", payload)
    require(legacy.returncode == 0 and not legacy.stderr, "legacy encoding failed")
    print(f"{len(payload)} payload bytes, {len(grid)} protected grid bytes, {len(legacy.stdout)} legacy grid bytes.")
    print("Masked rows | Starts | Interleaved (CLI) | Contiguous (erasure model)")
    for count in range(1, 6):
        starts = 13 - count
        interleaved = 0
        for start in range(starts):
            recovered = run(executable, "recover", erase_rows(grid, start, count))
            if count <= 4:
                require(
                    recovered.returncode == 0 and not recovered.stderr and recovered.stdout == payload,
                    f"failed to recover {count} erased rows starting at row {start + 1}",
                )
                interleaved += 1
            else:
                require(
                    recovered.returncode == 1 and not recovered.stdout and b"more than four" in recovered.stderr,
                    f"unexpected result for {count} erased rows starting at row {start + 1}",
                )
        contiguous = sum(contiguous_words_survive(start, count, 32) for start in range(starts))
        print(f"{count:11} | {starts:6} | {interleaved:17} | {contiguous:26}")

    # A one-cell-wide vertical cut can exhaust one codeword while leaving the rest of the band intact.
    lines = grid.splitlines()
    for row in range(1, 6):
        lines[row] = lines[row][:1] + b"??" + lines[row][3:]
    slit = run(executable, "recover", b"\n".join(lines) + b"\n")
    require(
        slit.returncode == 1 and not slit.stdout and b"more than four" in slit.stderr,
        "expected a five-symbol failure in one column",
    )
    print("A five-row slit in one byte column fails with no payload output.")

    if args.output_dir:
        args.output_dir.mkdir(parents=True, exist_ok=True)
        (args.output_dir / "pristine.bgr").write_bytes(grid)
        (args.output_dir / "four-rows-erased.bgr").write_bytes(erase_rows(grid, 0, 4))
        (args.output_dir / "five-rows-erased.bgr").write_bytes(erase_rows(grid, 0, 5))
        (args.output_dir / "original.bin").write_bytes(payload)

    if args.demo_svg:
        write_demo_svg(executable, args.demo_svg)

if __name__ == "__main__":
    main()
