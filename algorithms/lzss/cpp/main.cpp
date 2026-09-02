#include <cstdint>
#include <stdexcept>
#include <vector>

#include "compresskit/buffer_api.hpp"
#include "compresskit/checksum.hpp"
#include "compresskit/constants.hpp"
#include "compresskit/result.hpp"
#include "compresskit/serialization.hpp"

// LZSS (Lempel-Ziv-Storer-Szymanski) dictionary coding.
//
// Format:
// - Magic: 4 bytes "LZS2"
// - Symbol stream: groups of 8 symbols, each group prefixed by one flag byte
//   (bit i, LSB first: 1 = literal, 0 = match).
//   - literal: 1 raw byte
//   - match:   u16 LE = (dist - 1) (12 bits) | (len - 3) (4 bits)
//     dist is 1-based (1 = previous byte), max 4096 (4 KiB window), stored
//     as dist-1 so the 12-bit field covers the full window; len ranges 3..18.
// - CRC-32: 4 bytes LE, covering all preceding bytes; verified before parsing.
//
// The encoder uses a fixed-memory 3-byte-prefix hash table plus a ring of
// window-sized predecessor slots, so memory is O(1) regardless of input size.

namespace compresskit {

namespace {

constexpr int LZSS_WINDOW = 4096;   // 12-bit match distance
constexpr int LZSS_MIN_MATCH = 3;   // shorter matches cost more than literals
constexpr int LZSS_MAX_MATCH = 18;  // 4-bit length field (len - 3)
constexpr int LZSS_HASH_SIZE = 1 << 16;
constexpr int LZSS_MAX_CHAIN = 32;  // candidates examined per position

inline uint32_t lzss_hash3(const uint8_t* p) {
    // Multiplicative hash over the 3-byte prefix, taking the high 16 bits.
    return ((static_cast<uint32_t>(p[0]) << 16) | (static_cast<uint32_t>(p[1]) << 8) |
            static_cast<uint32_t>(p[2])) *
               2654435761u >>
           16;
}

}  // namespace

std::vector<uint8_t> lzss_encode_buffer(const std::vector<uint8_t>& input) {
    if (input.size() >= compresskit::MAX_RAW_SIZE) {
        throw std::runtime_error("LZSS: input too large");
    }
    std::vector<uint8_t> out;
    // Worst case is all literals: 1 byte per input byte + 1 flag per 8.
    out.reserve(input.size() + input.size() / 8 + compresskit::MAGIC_SIZE +
                compresskit::CRC32_SIZE + compresskit::INITIAL_ENCODE_OVERHEAD);
    compresskit::write_magic(out, compresskit::LZSS_MAGIC);

    if (input.empty()) {
        compresskit::append_crc32(out);
        return out;
    }

    std::vector<int32_t> head(LZSS_HASH_SIZE, -1);
    std::vector<int32_t> prev(LZSS_WINDOW, -1);

    auto insert = [&](int32_t pos) {
        if (pos + LZSS_MIN_MATCH > static_cast<int32_t>(input.size())) {
            return;
        }
        uint32_t h = lzss_hash3(input.data() + pos);
        prev[pos % LZSS_WINDOW] = head[h];
        head[h] = pos;
    };

    std::size_t pos = 0;
    int count = 0;
    int flags = 0;
    std::size_t flag_pos = out.size();
    out.push_back(0);  // placeholder flag byte

    auto emit_symbol = [&](bool is_literal, uint8_t byte, int32_t dist, int len) {
        if (is_literal) {
            flags |= (1 << count);
            out.push_back(byte);
        } else {
            uint32_t stored = static_cast<uint32_t>(dist) - 1;  // 0..4095 fits 12 bits
            out.push_back(static_cast<uint8_t>(stored & 0xFF));
            out.push_back(static_cast<uint8_t>((stored >> 8) | ((len - LZSS_MIN_MATCH) << 4)));
        }
        ++count;
        if (count == 8) {
            out[flag_pos] = static_cast<uint8_t>(flags);
            flag_pos = out.size();
            out.push_back(0);
            flags = 0;
            count = 0;
        }
    };

    while (pos < input.size()) {
        int best_len = 0;
        int32_t best_dist = 0;
        if (pos + static_cast<std::size_t>(LZSS_MIN_MATCH) <= input.size()) {
            uint32_t h = lzss_hash3(input.data() + pos);
            int32_t cand = head[h];
            int chain = 0;
            while (cand >= 0 && chain < LZSS_MAX_CHAIN) {
                int32_t dist = static_cast<int32_t>(pos) - cand;
                if (dist > LZSS_WINDOW) {
                    break;  // chain is newest-first, later candidates are older
                }
                if (pos + static_cast<std::size_t>(best_len) < input.size() &&
                    input[cand + best_len] == input[pos + best_len]) {
                    int len = 0;
                    while (len < LZSS_MAX_MATCH &&
                           pos + static_cast<std::size_t>(len) < input.size() &&
                           input[cand + len] == input[pos + len]) {
                        ++len;
                    }
                    if (len > best_len) {
                        best_len = len;
                        best_dist = dist;
                    }
                    if (len == LZSS_MAX_MATCH) {
                        break;
                    }
                }
                cand = prev[cand % LZSS_WINDOW];
                ++chain;
            }
        }

        if (best_len >= LZSS_MIN_MATCH) {
            emit_symbol(false, 0, best_dist, best_len);
            // Every position inside the match must enter the table so later
            // matches can extend across it.
            for (int k = 0; k < best_len; ++k) {
                insert(static_cast<int32_t>(pos) + k);
            }
            pos += best_len;
        } else {
            insert(static_cast<int32_t>(pos));
            emit_symbol(true, input[pos], 0, 0);
            ++pos;
        }
    }
    out[flag_pos] = static_cast<uint8_t>(flags);
    compresskit::append_crc32(out);
    return out;
}

std::vector<uint8_t> lzss_decode_buffer(const std::vector<uint8_t>& input) {
    compresskit::precheck_magic(input, compresskit::LZSS_MAGIC, "LZSS", false);
    std::size_t content = compresskit::verify_crc32(input, "LZSS");
    const uint8_t* data = input.data();
    std::size_t pos = 0;
    compresskit::verify_magic(data, content, pos, compresskit::LZSS_MAGIC, "LZSS", false);

    std::vector<uint8_t> out;
    while (pos < content) {
        uint8_t flags = data[pos++];
        for (int i = 0; i < 8; ++i) {
            if (pos >= content) {
                break;  // final group may declare fewer than 8 symbols
            }
            if (flags & (1 << i)) {
                if (out.size() >= compresskit::MAX_RAW_SIZE) {
                    throw compresskit::SizeLimitError("LZSS: output size limit exceeded");
                }
                out.push_back(data[pos++]);
            } else {
                if (pos + 2 > content) {
                    throw std::runtime_error("LZSS: truncated match pair");
                }
                uint32_t v = data[pos] | (static_cast<uint32_t>(data[pos + 1]) << 8);
                pos += 2;
                uint32_t dist = (v & 0xFFF) + 1;  // stored dist-1, 1-based distance
                uint32_t len = (v >> 12) + LZSS_MIN_MATCH;
                if (dist > out.size()) {
                    throw std::runtime_error("LZSS: match distance out of range");
                }
                if (out.size() + len > compresskit::MAX_RAW_SIZE) {
                    throw compresskit::SizeLimitError("LZSS: output size limit exceeded");
                }
                // Byte-by-byte copy handles overlapping matches (dist < len).
                for (uint32_t k = 0; k < len; ++k) {
                    out.push_back(out[out.size() - dist]);
                }
            }
        }
    }
    return out;
}

}  // namespace compresskit

#ifndef COMPRESSKIT_NO_MAIN
#include "compresskit/cli_launcher.hpp"

int main(int argc, char** argv) {
    compresskit::cli::Algorithm algo{compresskit::lzss_encode_buffer,
                                     compresskit::lzss_decode_buffer};
    return compresskit::cli::run(algo, argc, argv);
}
#endif
