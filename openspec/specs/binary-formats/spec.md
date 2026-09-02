# Binary Formats v2 Change Specification

## ADDED Requirements

### Requirement: Versioned algorithm magic
Each v2 encoder SHALL emit its approved 4-byte magic: `HFM2`, `AEN2`, `RCN2`, or `RLE2`.

#### Scenario: Encode each algorithm
- **WHEN** empty or representative input is encoded with an algorithm
- **THEN** the first four output bytes SHALL equal that algorithm's approved v2 magic

### Requirement: Deterministic legacy rejection
The v2 decoders SHALL distinguish recognizable legacy magic from v2 and reject it with an unsupported-legacy error before parsing a v2 body.

#### Scenario: Recognizable v1 archive
- **GIVEN** a frozen v1 Huffman, Arithmetic, or Range archive
- **WHEN** its v2 decoder opens it
- **THEN** the decoder SHALL return unsupported legacy format
- **AND** SHALL NOT report checksum mismatch as the primary error

#### Scenario: Legacy RLE without magic
- **GIVEN** a v1 RLE payload without identifying magic
- **WHEN** the v2 RLE decoder opens it
- **THEN** it SHALL return bad magic with expected `RLE2`

### Requirement: V2 integrity trailer
Every v2 stream SHALL end with a little-endian IEEE-compatible CRC-32 covering all bytes before the trailer.

#### Scenario: Corrupted v2 byte
- **GIVEN** a valid v2 archive with one covered byte modified
- **WHEN** it is decoded
- **THEN** decoding SHALL fail with checksum or an earlier structural error
- **AND** SHALL NOT return partial successful output

### Requirement: Exact size contract
The public specification and implementation SHALL use the same raw-input, decoded-output, and compressed-input boundaries.

#### Scenario: Boundary values
- **WHEN** input is one byte below, exactly at, or above each declared boundary
- **THEN** acceptance or size-limit rejection SHALL match the documented `<` or `<=` operator

### Requirement: Frozen format fixtures
The repository SHALL retain v1 rejection and v2 decoding fixtures with reproducible manifests for all supported algorithms.

#### Scenario: Run fixture suite
- **WHEN** the standard test command runs
- **THEN** every manifest case SHALL decode or reject with its declared result

### Requirement: Major release consistency
The project version, changelog and format documentation SHALL identify this breaking format generation as `2.0.0` and v2.

#### Scenario: Inspect release metadata
- **WHEN** maintainers inspect CMake/project version, changelog and current format pages
- **THEN** all SHALL describe the same v2 release boundary
