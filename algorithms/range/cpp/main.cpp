#include <cstdint>
#include <stdexcept>
#include <vector>

#include "compresskit/buffer_api.hpp"
#include "compresskit/checksum.hpp"
#include "compresskit/constants.hpp"
#include "compresskit/frequency_table.hpp"
#include "compresskit/result.hpp"
#include "compresskit/serialization.hpp"

// Range coder (32-bit state, byte renormalisation, carryless, static model).
// Format: "RCN2" + frequency table (LE) + range-coded byte stream.
//
// The coder tracks the half-open interval [low, low + range). Renormalisation
// keeps range >= RENORM_THRESHOLD (== MAX_TOTAL) so `range / total` never
// collapses a symbol sub-interval to zero. When low and low + range straddle
// a top-byte boundary while range is still narrow, the interval end is
// snapped onto that boundary (costing a negligible fraction of coding
// efficiency) so the shift can proceed. An earlier revision renormalised
// only while the top bytes of low and high matched, which could leave
// range < RENORM_THRESHOLD and silently corrupted near-incompressible data.

namespace compresskit {
namespace {

constexpr uint32_t MAX_TOTAL = compresskit::MAX_FREQ_TOTAL;
constexpr uint32_t RENORM_THRESHOLD = compresskit::MAX_FREQ_TOTAL;
constexpr int STATE_BYTES = 4;  // 32-bit state, flushed byte-by-byte
constexpr int TOP_BYTE_SHIFT = (STATE_BYTES - 1) * compresskit::BITS_PER_BYTE;  // 24

class RangeEncoder {
public:
    explicit RangeEncoder(std::vector<uint8_t>& out) : out_(out), low_(0), range_(UINT32_MAX) {}

    void encode_symbol(uint32_t symbol, const std::vector<uint32_t>& cumulative) {
        uint64_t total = cumulative.back();
        // Renormalisation guarantees range_ >= total, so r >= 1.
        uint64_t r = range_ / total;
        low_ += static_cast<uint32_t>(r * cumulative[symbol]);
        range_ = static_cast<uint32_t>(r * (cumulative[symbol + 1] - cumulative[symbol]));
        renorm();
    }

    void finish() {
        for (int i = 0; i < STATE_BYTES; ++i) {
            out_.push_back(static_cast<uint8_t>(low_ >> TOP_BYTE_SHIFT));
            low_ <<= compresskit::BITS_PER_BYTE;
        }
    }

private:
    void renorm() {
        while ((low_ ^ (low_ + range_)) < RENORM_THRESHOLD || range_ < RENORM_THRESHOLD) {
            if ((low_ ^ (low_ + range_)) >= RENORM_THRESHOLD) {
                // Narrow interval crossing a top-byte boundary: snap its end
                // to the boundary so one more byte can be emitted.
                range_ = static_cast<uint32_t>(-low_) & (RENORM_THRESHOLD - 1);
            }
            out_.push_back(static_cast<uint8_t>(low_ >> TOP_BYTE_SHIFT));
            low_ <<= compresskit::BITS_PER_BYTE;
            range_ <<= compresskit::BITS_PER_BYTE;
        }
    }

    std::vector<uint8_t>& out_;
    uint32_t low_, range_;
};

class RangeDecoder {
public:
    RangeDecoder(const uint8_t* data, std::size_t size)
        : data_(data), size_(size), pos_(0), low_(0), range_(UINT32_MAX), code_(0) {
        for (int i = 0; i < STATE_BYTES; ++i) {
            code_ = (code_ << compresskit::BITS_PER_BYTE) | read_byte();
        }
    }

    uint32_t decode_symbol(const std::vector<uint32_t>& cumulative) {
        uint64_t total = cumulative.back();
        uint64_t r = range_ / total;
        uint32_t value = static_cast<uint32_t>((code_ - low_) / r);
        uint32_t symbol = compresskit::find_symbol(cumulative, value);
        low_ += static_cast<uint32_t>(r * cumulative[symbol]);
        range_ = static_cast<uint32_t>(r * (cumulative[symbol + 1] - cumulative[symbol]));
        renorm();
        return symbol;
    }

private:
    void renorm() {
        while ((low_ ^ (low_ + range_)) < RENORM_THRESHOLD || range_ < RENORM_THRESHOLD) {
            if ((low_ ^ (low_ + range_)) >= RENORM_THRESHOLD) {
                range_ = static_cast<uint32_t>(-low_) & (RENORM_THRESHOLD - 1);
            }
            low_ <<= compresskit::BITS_PER_BYTE;
            range_ <<= compresskit::BITS_PER_BYTE;
            code_ = (code_ << compresskit::BITS_PER_BYTE) | read_byte();
        }
    }
    uint32_t read_byte() {
        if (pos_ < size_) {
            return static_cast<uint32_t>(data_[pos_++]);
        }
        return 0;
    }
    const uint8_t* data_;
    std::size_t size_;
    std::size_t pos_;
    uint32_t low_, range_, code_;
};

}  // namespace

std::vector<uint8_t> rangecoder_encode_buffer(const std::vector<uint8_t>& input) {
    if (input.size() >= compresskit::MAX_RAW_SIZE) {
        throw std::runtime_error("range: input too large");
    }
    std::vector<uint32_t> freq = compresskit::build_entropy_frequencies(input, MAX_TOTAL);
    std::vector<uint32_t> cumulative = compresskit::build_cumulative(freq);

    std::vector<uint8_t> out;
    out.reserve(input.size() + compresskit::INITIAL_ENCODE_OVERHEAD);
    compresskit::write_frequency_header(out, compresskit::RANGE_MAGIC, freq);

    RangeEncoder encoder(out);
    for (uint8_t b : input) {
        encoder.encode_symbol(static_cast<uint32_t>(b), cumulative);
    }
    encoder.encode_symbol(compresskit::EOF_SYMBOL, cumulative);
    encoder.finish();
    compresskit::append_crc32(out);
    return out;
}

std::vector<uint8_t> rangecoder_decode_buffer(const std::vector<uint8_t>& input) {
    compresskit::precheck_magic(input, compresskit::RANGE_MAGIC, "range");
    std::size_t content = compresskit::verify_crc32(input, "range");
    const uint8_t* data = input.data();
    std::size_t pos = 0;
    std::vector<uint32_t> freq = compresskit::read_magic_and_frequency_header(
        data, content, pos, compresskit::RANGE_MAGIC, "range");
    if (freq[compresskit::EOF_SYMBOL] == 0 || compresskit::frequency_total(freq) > MAX_TOTAL) {
        throw std::runtime_error("range: corrupt frequency table");
    }
    std::vector<uint32_t> cumulative = compresskit::build_cumulative(freq);

    // The encoder always flushes STATE_BYTES of final state, so a shorter
    // payload cannot come from a valid encoder.
    if (content - pos < STATE_BYTES) {
        throw std::runtime_error("range: truncated stream");
    }

    std::vector<uint8_t> out;
    RangeDecoder decoder(data + pos, content - pos);
    for (;;) {
        uint32_t sym = decoder.decode_symbol(cumulative);
        if (sym == compresskit::EOF_SYMBOL) {
            break;
        }
        if (out.size() >= compresskit::MAX_RAW_SIZE) {
            throw compresskit::SizeLimitError("range: output size limit exceeded");
        }
        out.push_back(static_cast<uint8_t>(sym));
    }
    return out;
}

}  // namespace compresskit

#ifndef COMPRESSKIT_NO_MAIN
#include "compresskit/cli_launcher.hpp"

int main(int argc, char** argv) {
    compresskit::cli::Algorithm algo{compresskit::rangecoder_encode_buffer,
                                     compresskit::rangecoder_decode_buffer};
    return compresskit::cli::run(algo, argc, argv);
}
#endif
