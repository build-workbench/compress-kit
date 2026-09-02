#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace compresskit {

// Symbol table size: 256 byte values + 1 EOF marker.
constexpr uint32_t SYMBOL_LIMIT = 257;
constexpr uint32_t EOF_SYMBOL = SYMBOL_LIMIT - 1;

// Size limits. Raw data (encode input) must be strictly below MAX_RAW_SIZE;
// the strict encode bound also keeps uint32 frequency counts from wrapping.
// Decoders refuse output that would exceed MAX_RAW_SIZE (exactly MAX_RAW_SIZE
// bytes is allowed). Compressed streams fed to a decoder may be larger than
// the raw data they encode (RLE expands incompressible input ~5x), so decode
// input gets its own, larger bound.
constexpr uint64_t MAX_RAW_SIZE = 1ULL * 1024 * 1024 * 1024;         // 1 GiB
constexpr uint64_t MAX_COMPRESSED_SIZE = 8ULL * 1024 * 1024 * 1024;  // 8 GiB

// Binary format element sizes.
constexpr std::size_t MAGIC_SIZE = 4;  // algorithm magic prefix length
constexpr std::size_t U32_SIZE = 4;    // little-endian uint32 serialisation
constexpr int BITS_PER_BYTE = 8;
constexpr uint32_t BYTE_VALUES = 1u << BITS_PER_BYTE;  // 256
constexpr uint32_t MAX_FREQ_TOTAL = 1u << 24;          // entropy tables; keeps range/total >= 1
constexpr std::size_t RLE_PAIR_SIZE = U32_SIZE + 1;    // count + value byte
constexpr std::size_t CRC32_SIZE = 4;                  // trailing CRC-32 checksum

// I/O tuneables.
constexpr std::size_t INITIAL_ENCODE_OVERHEAD = 2048;

// ── v2 algorithm magic numbers ─────────────────────────────────────────────
//
// v2 uses a clean 4-byte magic for every algorithm, including RLE (which had
// no magic in v1). Every v2 stream ends with a little-endian CRC-32 trailer.
//
// Format: magic (4B) + algorithm body + CRC-32 (4B LE)

constexpr char HUFFMAN_MAGIC[4] = {'H', 'F', 'M', '2'};
constexpr char ARITHMETIC_MAGIC[4] = {'A', 'E', 'N', '2'};
constexpr char RANGE_MAGIC[4] = {'R', 'C', 'N', '2'};
constexpr char RLE_MAGIC[4] = {'R', 'L', 'E', '2'};

// ── Legacy (v1) magic numbers ──────────────────────────────────────────────
//
// v1 Huffman/Arithmetic/Range used these magics without a CRC trailer.
// v1 RLE had no magic at all. The v2 decoders recognise the legacy magics
// and reject them with an explicit "unsupported legacy format" error.

constexpr char HUFFMAN_LEGACY_MAGIC[4] = {'H', 'F', 'M', 'N'};
constexpr char ARITHMETIC_LEGACY_MAGIC[4] = {'A', 'E', 'N', 'C'};
constexpr char RANGE_LEGACY_MAGIC[4] = {'R', 'C', 'N', 'C'};

// Classifies the leading 4 bytes of a compressed stream.
enum class MagicClass {
    V2,       // matches the expected v2 magic
    Legacy,   // matches a recognizable v1 magic (HFMN/AENC/RCNC)
    Unknown,  // does not match any known magic
};

inline bool bytes_equal(const uint8_t* data, const char* magic) {
    return std::memcmp(data, magic, MAGIC_SIZE) == 0;
}

// Returns Legacy if the leading bytes match any of the three v1 magics.
inline MagicClass classify_magic(const uint8_t* data, std::size_t size,
                                 const char* expected_v2_magic) {
    if (size < MAGIC_SIZE) {
        return MagicClass::Unknown;
    }
    if (bytes_equal(data, expected_v2_magic)) {
        return MagicClass::V2;
    }
    if (bytes_equal(data, HUFFMAN_LEGACY_MAGIC) || bytes_equal(data, ARITHMETIC_LEGACY_MAGIC) ||
        bytes_equal(data, RANGE_LEGACY_MAGIC)) {
        return MagicClass::Legacy;
    }
    return MagicClass::Unknown;
}

}  // namespace compresskit
