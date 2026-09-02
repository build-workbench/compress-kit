# CompressKit Agent Guide

## Project Identity

- Product: **CompressKit** - C++17 compression laboratory
- Repository: `build-workbench/compress-kit`
- Default branch: `main`

## Core Contract

Four algorithms (Huffman, Arithmetic, Range, RLE) implemented in C++17.
Binary format compatibility is the primary constraint.

**Magic Numbers** (v2):
| Algorithm | Magic |
|-----------|-------|
| Huffman | `HFM2` |
| Arithmetic | `AEN2` |
| Range Coder | `RCN2` |
| RLE | `RLE2` |

Every stream ends with a little-endian CRC-32 trailer (4 bytes) covering all
preceding bytes; decoders verify it before parsing. Legacy magics (`HFMN`,
`AENC`, `RCNC`) are rejected as unsupported; v1 RLE had no magic and is
rejected as bad magic.

## Validation Commands

| Command | Purpose |
|---------|---------|
| `make build` | Build all C++ CLIs (CMake) |
| `make test` | Unit tests + CLI smoke + frozen fixtures |
| `make lint` | clang-format dry-run |
| `make sanitize` | ASan/UBSan lifecycle tests |

## Key Constraints

- Maintain binary format compatibility (magic bytes, frequency table LE layout, RLE pair layout, CRC-32 trailer)
- Size limits: encode input strictly below 1 GiB; decode output no larger than 1 GiB; compressed decode input strictly below 8 GiB
- Error messages in code must be English
- User-facing documentation (README, docs/) must be in Simplified Chinese

## Change Policy

Binary format changes and new algorithms require careful design review.
Internal refactors and bug fixes that preserve existing contract may be
implemented directly.
