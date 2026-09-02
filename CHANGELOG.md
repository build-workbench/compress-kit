# Changelog

All notable user-facing changes to CompressKit are tracked here.

The project follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
style categories and uses semantic versioning for releases.

## [Unreleased]

### Added

- Frozen v1-rejection and v2-decode fixtures with SHA-256 manifests, run by `make test`.
- `make sanitize` (ASan/UBSan) and a matching CI job.
- `make bench` refreshes the docs benchmark snapshot on shared 10 MiB / 1 MiB corpora.
- `make stats` prints Shannon entropy versus each algorithm's bits/byte.
- CLI tools accept `--help` / `-h` to print usage.
- Decoders report size-limit violations as `ERR_SIZE_LIMIT` (previously misclassified as corruption).
- Four algorithm pages gain hands-on experiment sections (entropy bound, skewed distributions, degenerate inputs, CRC tampering).
- New page "经典 vs 现代" compares all algorithms against gzip/zstd/brotli/xz/bzip2 on the same corpora.
- New page "熵界对照" shows Shannon entropy versus each algorithm's bits/byte across all corpora.
- New FASTQ-style corpus `fastq_10MiB` (150 bp reads, genome-like base distribution), included in benchmarks and CLI smoke tests.
- **New algorithm LZSS** (`lzss_cpp`, magic `LZS2`): sliding-window dictionary coding with hash-chain matching, fixed O(1) auxiliary memory, coverage in lifecycle tests, CLI smoke, frozen fixtures, and benchmarks.

### Changed

- Benchmark charts no longer substitute an 8 KiB sample for Range Coder.
- Input files are size-checked before allocation, so oversized inputs fail without an OOM allocation.
- CLI tools print the error message for missing input files and failed writes instead of failing silently.
- Benchmark snapshot refreshed with the new FASTQ dataset and LZSS rows.
- "经典 vs 现代" and "熵界对照" pages include LZSS measurements.

## [2.0.0] - 2026-08-18

### Changed

- **BREAKING**: Project refactored to C++17-only. Go and Rust implementations removed.
- **BREAKING**: Build system migrated from raw g++ Makefile to CMake (`CMakeLists.txt` with static library target, 4 algorithm executables, and CTest integration).
- Buffer layer rewritten to use in-memory transforms instead of temporary files. `encode_buffer` / `decode_buffer` now take a `BufferTransform` function pointer directly.
- **Simplification**: Removed Streaming state-machine layer (`encoder.hpp` with `State`/`Encoder`/`Decoder` abstract classes, `BufferEncoder`/`BufferDecoder` wrappers). The state machine was a dead abstraction - algorithms never used the streaming interface.
- **Simplification**: Pruned `StatusCode` enum to only used values (`OK` / `ERR_CORRUPT` / `ERR_SIZE_LIMIT`); removed `INITIAL_DECODE_OVERHEAD` constant.
- **Simplification**: Merged CI workflows into a single `ci.yml` (removed `ci-docs.yml`, `codeql.yml`); `docs-pages.yml` later restored for the slimmed docs site.
- Huffman encoding now uses `uint64_t` code words instead of `std::string` for 8x density.
- Huffman decoding now uses 8-bit lookup table per internal node (~8x faster than bit-by-bit tree walk).
- Range Coder now uses shared `frequency_table` utilities (was duplicating scale/cumulative/header logic).
- `scale_frequencies` rewritten from O(N×M) decrement loop to O(N log N) proportional reduction.
- `build_cumulative` is now a plain prefix sum; all-zero/EOF-less tables are rejected at the entropy decode entry points instead.
- Centralized constants (`SYMBOL_LIMIT`, `EOF_SYMBOL`, magic bytes, size limits) in `constants.hpp`.
- Issue templates pruned of Go/Rust/OpenSpec/cross-language references; language scope reduced to C++17 / Python scripts / Docs (feature template adds CI).
- **BREAKING (all formats)**: Every compressed stream now ends with a little-endian CRC-32 trailer (4 bytes, zlib-compatible polynomial) covering all preceding bytes. Decoders verify the checksum before any parsing, so **any** bit corruption is rejected instead of silently decoding to wrong data. Streams written by earlier revisions are not readable (project decision: no backward-compatibility layer). See the v2 magic entry below.
- **BREAKING (size limits)**: Unified size contract. `MAX_INPUT_SIZE` / `MAX_OUTPUT_SIZE` are replaced by `MAX_RAW_SIZE` (1 GiB: encode input must be strictly smaller; decode output may not exceed) and `MAX_COMPRESSED_SIZE` (8 GiB: decode input bound, needed because RLE expands incompressible data ~5x). Previously a file between 1 GiB and 4 GiB could be encoded but never decoded.
- CLI smoke suite now round-trips the large corpus (`random_1MiB`, `random_10MiB`, `repetitive_10MiB`, `textlike_10MiB`) in addition to the small corpus; the large files were generated but never exercised before.
- **BREAKING (format identity)**: All four algorithm magics changed to v2: `HFM2`, `AEN2`, `RCN2`, `RLE2`. Legacy magics (`HFMN`, `AENC`, `RCNC`) are recognised and rejected with an explicit "unsupported legacy format" error before any body parsing. v1 RLE had no magic; non-`RLE2` input is rejected as "bad magic". This ensures format generation is determined before parsing, not guessed from CRC presence.
- **BREAKING (RLE magic)**: RLE now has a 4-byte magic (`RLE2`) for the first time. v1 RLE streams (no magic, no CRC) are not automatically recognisable and are rejected as bad magic.
- Project version set to `2.0.0` in `CMakeLists.txt` to mark the breaking format generation boundary.
- Decoders now classify input as v2, legacy, or unknown before parsing, preventing CRC mismatch from being reported as the primary error for legacy archives.

### Added

- `CMakeLists.txt` with static library target, 4 algorithm executables, and CTest integration.
- `algorithms/shared/cpp/include/compresskit/constants.hpp` for shared constants.
- Shared `count_frequencies`, `scale_frequencies`, `build_cumulative` utilities in `frequency_table.hpp`.
- `tests/metadata.py` validation metadata module (C++17-only: `LANGUAGE_ORDER = ("cpp",)`).
- `algorithms/shared/cpp/include/compresskit/serialization.hpp` - shared in-memory little-endian serialization helpers (`write_u32_le`, `write_magic`, `write_frequency_header`, `read_frequency_header`). Eliminates duplicated `push_u32` lambdas and `read_frequencies` across huffman/arithmetic/range (~90 lines removed).
- `algorithms/shared/cpp/include/compresskit/bit_io.hpp` - shared `BitWriter` / `BitReader`. Eliminates duplicated `BitWriter` class across huffman/arithmetic (~28 lines removed).
- Slimmed VitePress docs site (`docs/` + `docs-pages.yml` workflow) keeping only algorithms, architecture, and benchmarks content. API/architecture pages reflect the `BufferTransform`-based buffer layer and 3-value `StatusCode` enum.
- `algorithms/shared/cpp/include/compresskit/checksum.hpp` - header-only CRC-32 (constexpr table), `append_crc32` / `verify_crc32` helpers.
- Unit and CLI conformance cases asserting that single-byte corruption of any encoded stream is always detected (CRC path) and rejected with a non-zero exit code.

### Removed

- Go and Rust implementations and their cross-language conformance matrix / streaming API contract tests.
- OpenSpec, Cursor, and Claude skill meta-tooling directories.
- Governance docs (`CODE_OF_CONDUCT.md`, `SECURITY.md`, `CONTEXT.md`, `CONTRIBUTING.md`) and `.devcontainer/` - unnecessary for a hobby/learning project.
- Stale streaming API, state-machine academy, ADRs, contributing, project-structure, and bibliography doc pages.

### Fixed

- **BREAKING (RCNC bitstream)**: Range coder renormalisation now keeps `range >= 2^24` (carryless `range` formulation with boundary snap). The previous loop only shifted while the top bytes of `low`/`high` matched, which let `range` collapse below the precision floor and **silently corrupted near-incompressible data** (e.g. 1 MiB of random bytes "encoded" to ~3 KB and could not be decoded). Previously written RCNC payloads are not decodable by this revision; header layout and magic are unchanged.
- Range/arithmetic decoders could crash (division by zero) or run away on corrupt frequency tables. Decoders now validate the table: `freq[EOF] > 0` and 64-bit total `<= MAX_TOTAL`, and the symbol search never selects a zero-width interval.
- Encoding rejected only inputs `> 4 GiB`, so an input of exactly 2^32 identical bytes wrapped the `uint32_t` frequency counter to 0 and **silently encoded to a ~1 KB file decoding to empty** (total data loss with exit code 0). Fixed via the unified limit contract: raw input must be strictly smaller than 1 GiB, far below any `uint32_t` counting hazard.
- Huffman decoding of an all-zero / EOF-less frequency table silently returned empty output; it is now rejected as corrupt.
- Arithmetic and range decoders rejected header-only (zero-payload) streams by silently returning empty / decoding unboundedly; both now throw `truncated stream` (encoders always emit at least one payload byte).
- `test_lifecycle` relied on `assert()`, which `-DNDEBUG` compiles out in the default Release build - the suite passed without checking anything. Checks are now always active, and corrupt-input rejection cases were added.
- RLE encode was missing the input-size check the other three encoders enforce.
- Removed dead branches in `scale_frequencies` (unreachable `new_total == 0` fallback) and the now-unreachable single-leaf early return in the Huffman decoder.
- Huffman and Range docs listed v2 magic ASCII with v1 trailing bytes (`0x4E` / `0x43`); they now match `HFM2` / `RCN2`.
- Architecture buffer-layer size-limit text still documented the old 4 GiB encode cap; it now matches the 1 GiB raw / 8 GiB compressed contract.

### Clean Code: shared utilities & named constants

- RLE encode now uses shared `write_magic` and `write_u32_le` instead of inline copies.
- Removed unused `name` parameter from `compresskit::cli::run` (and all 4 algorithm call sites).
- Renamed `kInitialEncodeOverhead` (Google-style) to `INITIAL_ENCODE_OVERHEAD` (matches codebase UPPER_CASE convention for local constants).
- Added shared binary-format constants to `constants.hpp`: `MAGIC_SIZE`, `U32_SIZE`, `BITS_PER_BYTE`, `BYTE_VALUES`, `RLE_PAIR_SIZE`, `STREAM_READ_BUFFER_SIZE`, `INITIAL_ENCODE_OVERHEAD`, `INITIAL_DECODE_OVERHEAD`.
- Replaced bare magic numbers (`4`, `8`, `7`, `256`, `5`, `32 * 1024`, `0xFFFFFFFFu`) with named constants across huffman/arithmetic/range/rle decode paths, `serialization.hpp`, `frequency_table.cpp`, `bit_io.hpp`, and `buffer_api.cpp`.
- Named local constants: `STATE_BYTES = 4` (range coder 32-bit state width), `MAX_TREE_NODES = 2 * SYMBOL_LIMIT` (huffman worst-case node count), `EXPECTED_ARGC = 4` (program + mode + input + output), `TOP_BYTE_SHIFT = (STATE_BYTES - 1) * BITS_PER_BYTE`.
- Refactored `write_u32_le` / `write_magic` / `read_frequency_header` and rle count decode from unrolled byte shifts to `U32_SIZE` / `MAGIC_SIZE` loops.

## [1.0.0] - 2026-01-07

### Added

- Huffman Coding, Arithmetic Coding, Range Coder, and Run-Length Encoding implementations.
- C++17, Go, and Rust command-line tools for all four algorithms.
- Unified CLI shape: `<binary> <encode|decode> <input> <output>`.
- Cross-language file compatibility goals for educational verification.
- Test data generation scripts and benchmark scripts.
- VitePress documentation site with English and Chinese content.
- MIT license, contribution guide, code of conduct, security policy, issue templates, and pull request template.

### Security

- Documented maximum input size of 4 GiB.
- Documented maximum decoded output size of 1 GiB for decompression-bomb protection.

[Unreleased]: https://github.com/build-workbench/compress-kit/compare/v2.0.0...HEAD
[2.0.0]: https://github.com/build-workbench/compress-kit/compare/v1.0.0...v2.0.0
[1.0.0]: https://github.com/build-workbench/compress-kit/releases/tag/v1.0.0
