#pragma once

#include <cstdint>
#include <vector>

namespace compresskit {

// Memory-based encode/decode entry points for each algorithm.
// Each takes a byte buffer and returns the transformed buffer, throwing on error.
std::vector<uint8_t> huffman_encode_buffer(const std::vector<uint8_t>& input);
std::vector<uint8_t> huffman_decode_buffer(const std::vector<uint8_t>& input);
std::vector<uint8_t> arithmetic_encode_buffer(const std::vector<uint8_t>& input);
std::vector<uint8_t> arithmetic_decode_buffer(const std::vector<uint8_t>& input);
std::vector<uint8_t> rangecoder_encode_buffer(const std::vector<uint8_t>& input);
std::vector<uint8_t> rangecoder_decode_buffer(const std::vector<uint8_t>& input);
std::vector<uint8_t> rle_encode_buffer(const std::vector<uint8_t>& input);
std::vector<uint8_t> rle_decode_buffer(const std::vector<uint8_t>& input);
std::vector<uint8_t> lzss_encode_buffer(const std::vector<uint8_t>& input);
std::vector<uint8_t> lzss_decode_buffer(const std::vector<uint8_t>& input);

}  // namespace compresskit
