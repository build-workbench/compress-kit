#include <cstdint>
#include <stdexcept>
#include <vector>

#include "compresskit/bit_io.hpp"
#include "compresskit/buffer_api.hpp"
#include "compresskit/checksum.hpp"
#include "compresskit/constants.hpp"
#include "compresskit/frequency_table.hpp"
#include "compresskit/result.hpp"
#include "compresskit/serialization.hpp"

// Arithmetic coding (32-bit state, static model).
// Format: "AEN2" + frequency table (LE) + bitstream (MSB-first).

namespace compresskit {
namespace {

constexpr uint32_t MAX_TOTAL = compresskit::MAX_FREQ_TOTAL;
constexpr uint64_t STATE_BITS = 32;
constexpr uint64_t FULL_RANGE = 1ull << STATE_BITS;
constexpr uint64_t HALF_RANGE = FULL_RANGE >> 1;
constexpr uint64_t FIRST_QUARTER = HALF_RANGE >> 1;
constexpr uint64_t THIRD_QUARTER = FIRST_QUARTER * 3;

class ArithmeticEncoder {
public:
    explicit ArithmeticEncoder(compresskit::BitWriter& w)
        : writer_(w), low_(0), high_(FULL_RANGE - 1), pending_(0) {}

    void encode_symbol(uint32_t symbol, const std::vector<uint32_t>& cumulative) {
        uint64_t range = high_ - low_ + 1;
        uint64_t total = cumulative.back();
        uint64_t sym_low = cumulative[symbol];
        uint64_t sym_high = cumulative[symbol + 1];
        high_ = low_ + (range * sym_high) / total - 1;
        low_ = low_ + (range * sym_low) / total;
        for (;;) {
            if (high_ < HALF_RANGE) {
                output_bit(0);
            } else if (low_ >= HALF_RANGE) {
                output_bit(1);
                low_ -= HALF_RANGE;
                high_ -= HALF_RANGE;
            } else if (low_ >= FIRST_QUARTER && high_ < THIRD_QUARTER) {
                ++pending_;
                low_ -= FIRST_QUARTER;
                high_ -= FIRST_QUARTER;
            } else {
                break;
            }
            low_ <<= 1;
            high_ = (high_ << 1) | 1;
        }
    }

    void finish() {
        ++pending_;
        output_bit(low_ < FIRST_QUARTER ? 0 : 1);
        writer_.flush();
    }

private:
    void output_bit(int bit) {
        writer_.write_bit(bit);
        int complement = bit ^ 1;
        while (pending_ > 0) {
            writer_.write_bit(complement);
            --pending_;
        }
    }
    compresskit::BitWriter& writer_;
    uint64_t low_, high_, pending_;
};

class ArithmeticDecoder {
public:
    explicit ArithmeticDecoder(compresskit::BitReader& r)
        : reader_(r), low_(0), high_(FULL_RANGE - 1), code_(0) {
        for (uint64_t i = 0; i < STATE_BITS; ++i) {
            code_ = (code_ << 1) | static_cast<uint64_t>(reader_.read_bit());
        }
    }

    uint32_t decode_symbol(const std::vector<uint32_t>& cumulative) {
        uint64_t range = high_ - low_ + 1;
        uint64_t total = cumulative.back();
        uint64_t offset = code_ - low_;
        uint64_t value = ((offset + 1) * total - 1) / range;
        uint32_t symbol = compresskit::find_symbol(cumulative, value);
        uint64_t sym_low = cumulative[symbol];
        uint64_t sym_high = cumulative[symbol + 1];
        high_ = low_ + (range * sym_high) / total - 1;
        low_ = low_ + (range * sym_low) / total;
        for (;;) {
            if (high_ < HALF_RANGE) {
                // Already in the lower half: no additive correction.
            } else if (low_ >= HALF_RANGE) {
                low_ -= HALF_RANGE;
                high_ -= HALF_RANGE;
                code_ -= HALF_RANGE;
            } else if (low_ >= FIRST_QUARTER && high_ < THIRD_QUARTER) {
                low_ -= FIRST_QUARTER;
                high_ -= FIRST_QUARTER;
                code_ -= FIRST_QUARTER;
            } else {
                break;
            }
            low_ <<= 1;
            high_ = (high_ << 1) | 1;
            code_ = (code_ << 1) | static_cast<uint64_t>(reader_.read_bit());
        }
        return symbol;
    }

private:
    compresskit::BitReader& reader_;
    uint64_t low_, high_, code_;
};

}  // namespace

std::vector<uint8_t> arithmetic_encode_buffer(const std::vector<uint8_t>& input) {
    if (input.size() >= compresskit::MAX_RAW_SIZE) {
        throw std::runtime_error("arithmetic: input too large");
    }
    std::vector<uint32_t> freq = compresskit::build_entropy_frequencies(input, MAX_TOTAL);
    std::vector<uint32_t> cumulative = compresskit::build_cumulative(freq);

    std::vector<uint8_t> out;
    out.reserve(input.size() + compresskit::INITIAL_ENCODE_OVERHEAD);
    compresskit::write_frequency_header(out, compresskit::ARITHMETIC_MAGIC, freq);

    compresskit::BitWriter writer;
    ArithmeticEncoder encoder(writer);
    for (uint8_t b : input) {
        encoder.encode_symbol(static_cast<uint32_t>(b), cumulative);
    }
    encoder.encode_symbol(compresskit::EOF_SYMBOL, cumulative);
    encoder.finish();

    std::vector<uint8_t> bits = writer.finish();
    out.insert(out.end(), bits.begin(), bits.end());
    compresskit::append_crc32(out);
    return out;
}

std::vector<uint8_t> arithmetic_decode_buffer(const std::vector<uint8_t>& input) {
    compresskit::precheck_magic(input, compresskit::ARITHMETIC_MAGIC, "arithmetic");
    std::size_t content = compresskit::verify_crc32(input, "arithmetic");
    const uint8_t* data = input.data();
    std::size_t pos = 0;
    std::vector<uint32_t> freq = compresskit::read_magic_and_frequency_header(
        data, content, pos, compresskit::ARITHMETIC_MAGIC, "arithmetic");
    if (freq[compresskit::EOF_SYMBOL] == 0 || compresskit::frequency_total(freq) > MAX_TOTAL) {
        throw std::runtime_error("arithmetic: corrupt frequency table");
    }
    // finish() always emits at least one bit, so a valid stream has payload.
    if (pos >= content) {
        throw std::runtime_error("arithmetic: truncated stream");
    }
    std::vector<uint32_t> cumulative = compresskit::build_cumulative(freq);

    std::vector<uint8_t> out;
    compresskit::BitReader reader(data + pos, content - pos);
    ArithmeticDecoder decoder(reader);

    for (;;) {
        uint32_t sym = decoder.decode_symbol(cumulative);
        if (sym == compresskit::EOF_SYMBOL) {
            break;
        }
        if (out.size() >= compresskit::MAX_RAW_SIZE) {
            throw compresskit::SizeLimitError("arithmetic: output size limit exceeded");
        }
        out.push_back(static_cast<uint8_t>(sym));
    }
    return out;
}

}  // namespace compresskit

#ifndef COMPRESSKIT_NO_MAIN
#include "compresskit/cli_launcher.hpp"

int main(int argc, char** argv) {
    compresskit::cli::Algorithm algo{compresskit::arithmetic_encode_buffer,
                                     compresskit::arithmetic_decode_buffer};
    return compresskit::cli::run(algo, argc, argv);
}
#endif
