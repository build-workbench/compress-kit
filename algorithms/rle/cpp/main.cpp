#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "compresskit/buffer_api.hpp"
#include "compresskit/checksum.hpp"
#include "compresskit/constants.hpp"
#include "compresskit/result.hpp"
#include "compresskit/serialization.hpp"

// Run-Length encoding.
// Format:
// - Magic: 4 bytes "RLE2"
// - (count: uint32 LE, value: byte) pairs until EOF; count must be > 0.

namespace compresskit {

std::vector<uint8_t> rle_encode_buffer(const std::vector<uint8_t>& input) {
    if (input.size() >= compresskit::MAX_RAW_SIZE) {
        throw std::runtime_error("RLE: input too large");
    }
    std::vector<uint8_t> out;
    // RLE can expand incompressible input ~5x (one RLE_PAIR_SIZE pair per byte).
    out.reserve(input.size() / 8 + compresskit::MAGIC_SIZE + compresskit::RLE_PAIR_SIZE +
                compresskit::INITIAL_ENCODE_OVERHEAD);
    compresskit::write_magic(out, compresskit::RLE_MAGIC);

    if (input.empty()) {
        compresskit::append_crc32(out);
        return out;
    }

    auto emit_run = [&](uint32_t n, uint8_t value) {
        compresskit::write_u32_le(out, n);
        out.push_back(value);
    };

    uint8_t current = input[0];
    uint32_t count = 1;
    for (std::size_t i = 1; i < input.size(); ++i) {
        if (input[i] == current && count < UINT32_MAX) {
            ++count;
        } else {
            emit_run(count, current);
            current = input[i];
            count = 1;
        }
    }
    emit_run(count, current);
    compresskit::append_crc32(out);
    return out;
}

std::vector<uint8_t> rle_decode_buffer(const std::vector<uint8_t>& input) {
    compresskit::precheck_magic(input, compresskit::RLE_MAGIC, "RLE", false);
    std::size_t content = compresskit::verify_crc32(input, "RLE");
    const uint8_t* data = input.data();
    std::size_t pos = 0;
    compresskit::verify_magic(data, content, pos, compresskit::RLE_MAGIC, "RLE", false);

    std::vector<uint8_t> out;
    while (pos < content) {
        if (pos + compresskit::RLE_PAIR_SIZE > content) {
            throw std::runtime_error("RLE: truncated count+value pair");
        }
        uint32_t count = compresskit::read_u32_le(data + pos);
        uint8_t value = data[pos + compresskit::U32_SIZE];
        pos += compresskit::RLE_PAIR_SIZE;
        if (count == 0) {
            throw std::runtime_error("RLE: count must not be 0");
        }
        if (out.size() + count > compresskit::MAX_RAW_SIZE) {
            throw compresskit::SizeLimitError("RLE: output size limit exceeded");
        }
        out.insert(out.end(), count, value);
    }
    return out;
}

}  // namespace compresskit

#ifndef COMPRESSKIT_NO_MAIN
#include "compresskit/cli_launcher.hpp"

int main(int argc, char** argv) {
    compresskit::cli::Algorithm algo{compresskit::rle_encode_buffer,
                                     compresskit::rle_decode_buffer};
    return compresskit::cli::run(algo, argc, argv);
}
#endif
