#include "compresskit/buffer_api.hpp"

#include <cstdio>
#include <fstream>
#include <stdexcept>

#include "compresskit/constants.hpp"

namespace compresskit {
namespace {

bool write_file(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }
    if (!data.empty()) {
        out.write(reinterpret_cast<const char*>(data.data()),
                  static_cast<std::streamsize>(data.size()));
    }
    return static_cast<bool>(out);
}

// Reads the whole file into memory, rejecting files at or above `max_size`
// before allocating, so oversized inputs fail without an OOM allocation.
std::vector<uint8_t> read_file(const std::string& path, uint64_t max_size) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) {
        throw std::runtime_error("cannot open input file");
    }
    std::streampos size = in.tellg();
    if (size < 0) {
        throw std::runtime_error("cannot determine input file size");
    }
    if (static_cast<uint64_t>(size) >= max_size) {
        throw SizeLimitError("input file exceeds size limit");
    }
    std::vector<uint8_t> data(static_cast<std::size_t>(size));
    in.seekg(0, std::ios::beg);
    if (!data.empty()) {
        in.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
        if (!in) {
            throw std::runtime_error("failed to read input file");
        }
    }
    return data;
}

template <typename Layer>
bool apply_file(BufferTransform transform, const std::string& input_path,
                const std::string& output_path, Layer layer, uint64_t max_input_size) {
    try {
        std::vector<uint8_t> input = read_file(input_path, max_input_size);
        Result<std::vector<uint8_t>> result = layer(transform, input);
        if (!result.ok()) {
            return false;
        }
        if (!write_file(output_path, result.value)) {
            std::fprintf(stderr, "failed to write output file\n");
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return false;
    }
}

}  // namespace

Result<std::vector<uint8_t>> encode_buffer(BufferTransform transform,
                                           const std::vector<uint8_t>& input) {
    if (input.size() >= MAX_RAW_SIZE) {
        std::fprintf(stderr, "encode failed: input exceeds size limit\n");
        return {StatusCode::ERR_SIZE_LIMIT, {}};
    }
    try {
        std::vector<uint8_t> out = transform(input);
        return {StatusCode::OK, std::move(out)};
    } catch (const std::exception& e) {
        std::fprintf(stderr, "encode failed: %s\n", e.what());
        return {StatusCode::ERR_CORRUPT, {}};
    }
}

Result<std::vector<uint8_t>> decode_buffer(BufferTransform transform,
                                           const std::vector<uint8_t>& input) {
    if (input.size() >= MAX_COMPRESSED_SIZE) {
        std::fprintf(stderr, "decode failed: input exceeds size limit\n");
        return {StatusCode::ERR_SIZE_LIMIT, {}};
    }
    try {
        std::vector<uint8_t> out = transform(input);
        // Decoders already refuse to grow past MAX_RAW_SIZE internally, but
        // keep the post-condition for any future transform that does not.
        if (out.size() > MAX_RAW_SIZE) {
            std::fprintf(stderr, "decode failed: output exceeds size limit\n");
            return {StatusCode::ERR_SIZE_LIMIT, {}};
        }
        return {StatusCode::OK, std::move(out)};
    } catch (const SizeLimitError& e) {
        std::fprintf(stderr, "decode failed: %s\n", e.what());
        return {StatusCode::ERR_SIZE_LIMIT, {}};
    } catch (const std::exception& e) {
        std::fprintf(stderr, "decode failed: %s\n", e.what());
        return {StatusCode::ERR_CORRUPT, {}};
    }
}

bool encode_file_via_buffer(BufferTransform transform, const std::string& input_path,
                            const std::string& output_path) {
    return apply_file(transform, input_path, output_path, encode_buffer, MAX_RAW_SIZE);
}

bool decode_file_via_buffer(BufferTransform transform, const std::string& input_path,
                            const std::string& output_path) {
    return apply_file(transform, input_path, output_path, decode_buffer, MAX_COMPRESSED_SIZE);
}

}  // namespace compresskit
