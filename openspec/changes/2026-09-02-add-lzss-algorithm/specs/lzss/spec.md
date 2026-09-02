# LZSS Change Specification

## ADDED Requirements

### Requirement: Versioned algorithm magic
The LZSS encoder SHALL emit the 4-byte magic `LZS2` as the first bytes of every stream.

#### Scenario: Encode empty and representative input
- **WHEN** empty or representative input is encoded with LZSS
- **THEN** the first four output bytes SHALL equal `LZS2`

### Requirement: V2 integrity trailer
Every LZSS stream SHALL end with a little-endian CRC-32 covering all bytes before the trailer, verified before parsing.

#### Scenario: Corrupted LZSS byte
- **GIVEN** a valid LZSS archive with one covered byte modified
- **WHEN** it is decoded
- **THEN** decoding SHALL fail with a checksum or an earlier structural error
- **AND** SHALL NOT return partial successful output

### Requirement: Bad magic rejection
The LZSS decoder SHALL reject any stream whose leading 4 bytes are not `LZS2` with a bad-magic error before parsing the body.

#### Scenario: Unknown magic input
- **GIVEN** a stream with a non-`LZS2` magic prefix
- **WHEN** the LZSS decoder opens it
- **THEN** it SHALL return `bad magic`

### Requirement: Match distance validity
Decoded match pairs SHALL only reference bytes already emitted by the decoder.

#### Scenario: Out-of-range distance
- **GIVEN** an LZSS stream containing a match whose distance exceeds the decoded output size, or is zero
- **WHEN** it is decoded
- **THEN** decoding SHALL fail with `match distance out of range`

### Requirement: Exact size contract
LZSS SHALL obey the shared raw-data and compressed-input boundaries: encode input strictly below 1 GiB, decode output no larger than 1 GiB, compressed decode input strictly below 8 GiB.

#### Scenario: Boundary values
- **WHEN** input is one byte below, exactly at, or above each declared boundary
- **THEN** acceptance or size-limit rejection SHALL match the documented `<` or `<=` operator

### Requirement: Frozen format fixtures
The repository SHALL retain LZSS decode fixtures with reproducible manifests for empty, single-byte, and ASCII inputs.

#### Scenario: Run fixture suite
- **WHEN** the standard test command runs
- **THEN** every manifest case SHALL decode with its declared result
