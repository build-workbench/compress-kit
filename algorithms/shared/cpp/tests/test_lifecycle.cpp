#include <cstdint>
#include <cstdio>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include "compresskit/algorithms.hpp"
#include "compresskit/buffer_api.hpp"
#include "compresskit/constants.hpp"
#include "compresskit/frequency_table.hpp"
#include "compresskit/serialization.hpp"

namespace {

// Checks stay active in Release builds (plain assert() is compiled out by
// -DNDEBUG, which silently disabled this suite in the default build).
int failures = 0;

void check(bool cond, const char* expr, int line) {
    if (!cond) {
        std::fprintf(stderr, "FAIL test_lifecycle.cpp:%d: %s\n", line, expr);
        ++failures;
    }
}

#define CHECK(cond) check((cond), #cond, __LINE__)

struct AlgorithmCase {
    const char* name;
    const char* magic;
    compresskit::BufferTransform encode;
    compresskit::BufferTransform decode;
    bool has_frequency_header;
};

void test_roundtrip(const AlgorithmCase& algo, const std::vector<uint8_t>& input) {
    auto encoded = compresskit::encode_buffer(algo.encode, input);
    CHECK(encoded.status == compresskit::StatusCode::OK);
    if (!encoded.ok()) {
        return;
    }
    auto decoded = compresskit::decode_buffer(algo.decode, encoded.value);
    CHECK(decoded.status == compresskit::StatusCode::OK);
    CHECK(decoded.value == input);
}

template <typename Fn>
void expect_throw(const std::string& label, Fn fn) {
    try {
        fn();
        std::fprintf(stderr, "FAIL %s: expected exception, got success\n", label.c_str());
        ++failures;
    } catch (const std::exception&) {
        // Rejection is the contract for corrupt input.
    }
}

std::vector<uint8_t> header_only(const char* magic, const std::vector<uint32_t>& freq) {
    std::vector<uint8_t> out;
    compresskit::write_frequency_header(out, magic, freq);
    return out;
}

std::vector<uint8_t> make_sequential(std::size_t n) {
    std::vector<uint8_t> v(n);
    std::iota(v.begin(), v.end(), uint8_t{0});
    return v;
}

std::vector<uint8_t> make_lcg(std::size_t n) {
    std::vector<uint8_t> v(n);
    uint32_t state = 0xDEADBEEF;
    for (std::size_t i = 0; i < n; ++i) {
        state = state * 1664525u + 1013904223u;
        v[i] = static_cast<uint8_t>(state >> 24);
    }
    return v;
}

void test_rejects_corrupt_input(const AlgorithmCase& algo) {
    const std::string label = algo.name;

    expect_throw(label + ": bad magic", [&] {
        std::vector<uint8_t> garbage = {'X', 'X', 'X', 'X', 0, 0, 0, 0};
        algo.decode(garbage);
    });

    // Frequency tables must carry an EOF symbol; without one the entropy
    // decoders can never reach end-of-stream.
    if (algo.has_frequency_header) {
        std::vector<uint32_t> no_eof(compresskit::SYMBOL_LIMIT, 1);
        no_eof[compresskit::EOF_SYMBOL] = 0;
        expect_throw(label + ": frequency table without EOF",
                     [&] { algo.decode(header_only(algo.magic, no_eof)); });
        std::vector<uint32_t> zeros(compresskit::SYMBOL_LIMIT, 0);
        expect_throw(label + ": all-zero frequency table",
                     [&] { algo.decode(header_only(algo.magic, zeros)); });
    }

    // Truncating the payload of a real stream must fail loudly.
    std::vector<uint8_t> data(1024);
    std::iota(data.begin(), data.end(), uint8_t{0});
    auto encoded = algo.encode(data);
    CHECK(encoded.size() > 16);
    expect_throw(label + ": truncated stream",
                 [&] { algo.decode(std::vector<uint8_t>(encoded.begin(), encoded.begin() + 16)); });

    // The trailing CRC-32 must catch any single-byte corruption, anywhere
    // in the stream (header, payload, or the checksum itself).
    for (std::size_t pos : {std::size_t{0}, encoded.size() / 2, encoded.size() - 1}) {
        auto corrupted = encoded;
        corrupted[pos] ^= 0x40;
        expect_throw(label + ": corrupted byte detected", [&] { algo.decode(corrupted); });
    }
}

}  // namespace

int main() {
    const AlgorithmCase algorithms[] = {
        {"Huffman", compresskit::HUFFMAN_MAGIC, compresskit::huffman_encode_buffer,
         compresskit::huffman_decode_buffer, true},
        {"Arithmetic", compresskit::ARITHMETIC_MAGIC, compresskit::arithmetic_encode_buffer,
         compresskit::arithmetic_decode_buffer, true},
        {"Range", compresskit::RANGE_MAGIC, compresskit::rangecoder_encode_buffer,
         compresskit::rangecoder_decode_buffer, true},
        {"RLE", compresskit::RLE_MAGIC, compresskit::rle_encode_buffer,
         compresskit::rle_decode_buffer, false},
        {"LZSS", compresskit::LZSS_MAGIC, compresskit::lzss_encode_buffer,
         compresskit::lzss_decode_buffer, false},
    };

    struct Corpus {
        const char* label;
        std::vector<uint8_t> data;
    };

    std::vector<uint8_t> all_256(256);
    std::iota(all_256.begin(), all_256.end(), 0);

    const Corpus corpus[] = {
        {"empty", {}},
        {"single_byte", {0x42}},
        {"ascii", {'s', 't', 'r', 'e', 'a', 'm', '-', 'a', 'p', 'i'}},
        {"all_same", std::vector<uint8_t>(4096, 0xAB)},
        {"all_256", all_256},
        {"sequential_1k", make_sequential(1024)},
        {"lcg_64k", make_lcg(65536)},
        {"lcg_1mib", make_lcg(1 << 20)},
    };

    int passed = 0;
    for (const auto& algo : algorithms) {
        for (const auto& c : corpus) {
            test_roundtrip(algo, c.data);
            if (failures == 0) {
                ++passed;
                std::printf("PASS %-12s %s\n", algo.name, c.label);
            }
        }
        test_rejects_corrupt_input(algo);
        if (failures == 0) {
            std::printf("PASS %-12s corrupt-input rejection\n", algo.name);
        }
    }

    // RLE pair validation.
    expect_throw("RLE: count=0 pair", [] {
        std::vector<uint8_t> f;
        compresskit::write_magic(f, compresskit::RLE_MAGIC);
        compresskit::write_u32_le(f, 0);
        f.push_back('A');
        compresskit::rle_decode_buffer(f);
    });
    expect_throw("RLE: count above output limit", [] {
        std::vector<uint8_t> f;
        compresskit::write_magic(f, compresskit::RLE_MAGIC);
        compresskit::write_u32_le(f, UINT32_MAX);
        f.push_back('A');
        compresskit::rle_decode_buffer(f);
    });

    // Size contract: keep the documented 1 GiB / 8 GiB bounds without allocating them.
    CHECK(compresskit::MAX_RAW_SIZE == 1ULL * 1024 * 1024 * 1024);
    CHECK(compresskit::MAX_COMPRESSED_SIZE == 8ULL * 1024 * 1024 * 1024);
    CHECK(compresskit::MAX_RAW_SIZE < compresskit::MAX_COMPRESSED_SIZE);

    // v2 magic exact bytes.
    CHECK(compresskit::HUFFMAN_MAGIC[0] == 'H');
    CHECK(compresskit::HUFFMAN_MAGIC[3] == '2');
    CHECK(compresskit::ARITHMETIC_MAGIC[0] == 'A');
    CHECK(compresskit::ARITHMETIC_MAGIC[3] == '2');
    CHECK(compresskit::RANGE_MAGIC[0] == 'R');
    CHECK(compresskit::RANGE_MAGIC[3] == '2');
    CHECK(compresskit::RLE_MAGIC[0] == 'R');
    CHECK(compresskit::RLE_MAGIC[3] == '2');
    CHECK(compresskit::LZSS_MAGIC[0] == 'L');
    CHECK(compresskit::LZSS_MAGIC[1] == 'Z');
    CHECK(compresskit::LZSS_MAGIC[3] == '2');

    // Legacy magic rejection: Huffman/Arithmetic/Range v1 archives must be
    // rejected with "unsupported legacy format", not "checksum mismatch".
    auto make_legacy_input = [](const char* legacy_magic) {
        std::vector<uint8_t> out;
        compresskit::write_magic(out, legacy_magic);
        // Add some bytes so it's long enough to not be "too short".
        for (int i = 0; i < 32; ++i) {
            out.push_back(0);
        }
        return out;
    };

    expect_throw("Huffman: legacy HFMN rejected", [&] {
        compresskit::huffman_decode_buffer(make_legacy_input(compresskit::HUFFMAN_LEGACY_MAGIC));
    });
    expect_throw("Arithmetic: legacy AENC rejected", [&] {
        compresskit::arithmetic_decode_buffer(
            make_legacy_input(compresskit::ARITHMETIC_LEGACY_MAGIC));
    });
    expect_throw("Range: legacy RCNC rejected", [&] {
        compresskit::rangecoder_decode_buffer(make_legacy_input(compresskit::RANGE_LEGACY_MAGIC));
    });

    // Verify the error message says "legacy", not "checksum" or "bad magic".
    try {
        compresskit::huffman_decode_buffer(make_legacy_input(compresskit::HUFFMAN_LEGACY_MAGIC));
        CHECK(false);
    } catch (const std::exception& e) {
        std::string msg = e.what();
        CHECK(msg.find("legacy") != std::string::npos);
        CHECK(msg.find("checksum") == std::string::npos);
    }

    // RLE v1 had no magic; any non-RLE2 input is "bad magic".
    expect_throw("RLE: non-RLE2 rejected as bad magic", [&] {
        std::vector<uint8_t> f = {'R', 'L', 'E', '\x00', 0, 0, 0, 0};
        compresskit::rle_decode_buffer(f);
    });

    // Verify the RLE error says "bad magic", not "checksum".
    try {
        std::vector<uint8_t> f = {'R', 'L', 'E', '\x00', 0, 0, 0, 0, 1, 0, 0, 0, 'A'};
        compresskit::rle_decode_buffer(f);
        CHECK(false);
    } catch (const std::exception& e) {
        std::string msg = e.what();
        CHECK(msg.find("bad magic") != std::string::npos);
    }

    if (failures > 0) {
        std::fprintf(stderr, "test_lifecycle: %d check(s) FAILED\n", failures);
        return 1;
    }
    std::printf("test_lifecycle: %d round-trip(s) + corrupt-input checks passed\n", passed);
    return 0;
}
