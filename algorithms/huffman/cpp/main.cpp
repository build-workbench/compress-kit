#include <algorithm>
#include <array>
#include <cstdint>
#include <queue>
#include <stdexcept>
#include <vector>

#include "compresskit/bit_io.hpp"
#include "compresskit/buffer_api.hpp"
#include "compresskit/checksum.hpp"
#include "compresskit/constants.hpp"
#include "compresskit/frequency_table.hpp"
#include "compresskit/result.hpp"
#include "compresskit/serialization.hpp"

// Huffman coding (static model, prefix codes).
// Format: "HFM2" + frequency table (LE) + bitstream (MSB-first).
//
// Performance notes:
// - Codes stored as {uint64_t bits, uint8_t len} (8x denser than std::string).
// - Decode uses an 8-bit lookup table per internal node (~8x faster than
//   bit-by-bit tree walk).

namespace compresskit {
namespace {

// Huffman code: bits stored with the first emitted bit at position (len-1).
// With uint32 frequencies over 257 symbols the worst-case tree depth is
// bounded by log_phi(257 * 2^32) < 60, so a 64-bit codeword always suffices.
struct Code {
    uint64_t bits = 0;
    uint8_t len = 0;
};

// Tree node arena: children are indices (-1 = none).
struct Node {
    uint32_t symbol = 0;
    uint64_t freq = 0;
    int32_t left = -1;
    int32_t right = -1;
};

// Worst-case node count: every symbol becomes a leaf, plus internal merges.
constexpr std::size_t MAX_TREE_NODES = 2 * compresskit::SYMBOL_LIMIT;

struct HeapItem {
    uint64_t freq;
    uint32_t symbol;
    int32_t index;
};

struct HeapCmp {
    bool operator()(const HeapItem& a, const HeapItem& b) const {
        if (a.freq != b.freq)
            return a.freq > b.freq;
        return a.symbol > b.symbol;
    }
};

bool is_leaf(const std::vector<Node>& nodes, int32_t idx) {
    return nodes[idx].left < 0 && nodes[idx].right < 0;
}

int32_t build_tree(const std::vector<uint32_t>& freq, std::vector<Node>& nodes) {
    std::priority_queue<HeapItem, std::vector<HeapItem>, HeapCmp> pq;
    for (uint32_t s = 0; s < compresskit::SYMBOL_LIMIT; ++s) {
        if (freq[s] == 0)
            continue;
        int32_t idx = static_cast<int32_t>(nodes.size());
        nodes.push_back({s, freq[s], -1, -1});
        pq.push({freq[s], s, idx});
    }
    if (pq.empty()) {
        int32_t idx = static_cast<int32_t>(nodes.size());
        nodes.push_back({compresskit::EOF_SYMBOL, 1, -1, -1});
        return idx;
    }
    if (pq.size() == 1) {
        HeapItem only = pq.top();
        pq.pop();
        int32_t parent = static_cast<int32_t>(nodes.size());
        nodes.push_back({only.symbol, only.freq, only.index, -1});
        return parent;
    }
    while (pq.size() > 1) {
        HeapItem a = pq.top();
        pq.pop();
        HeapItem b = pq.top();
        pq.pop();
        uint32_t min_sym = std::min(a.symbol, b.symbol);
        int32_t parent = static_cast<int32_t>(nodes.size());
        nodes.push_back({min_sym, a.freq + b.freq, a.index, b.index});
        pq.push({a.freq + b.freq, min_sym, parent});
    }
    return pq.top().index;
}

void build_codes(const std::vector<Node>& nodes, int32_t idx, Code* codes, uint64_t prefix_bits,
                 uint8_t prefix_len) {
    if (idx < 0)
        return;
    if (is_leaf(nodes, idx)) {
        if (prefix_len == 0) {
            codes[nodes[idx].symbol] = {0, 1};
        } else {
            codes[nodes[idx].symbol] = {prefix_bits, prefix_len};
        }
        return;
    }
    const Node& n = nodes[idx];
    build_codes(nodes, n.left, codes, prefix_bits << 1, prefix_len + 1);
    build_codes(nodes, n.right, codes, (prefix_bits << 1) | 1, prefix_len + 1);
}

// 8-bit decode table entry: result of consuming one byte starting at `node`.
struct DecodeEntry {
    uint8_t count = 0;  // number of symbols emitted (0..BITS_PER_BYTE)
    uint32_t symbols[compresskit::BITS_PER_BYTE] = {};  // 0..256, need 9 bits
    int32_t next = 0;                                   // node index to continue from
};

void build_decode_table(const std::vector<Node>& nodes, int32_t root,
                        std::vector<std::array<DecodeEntry, compresskit::BYTE_VALUES>>& table) {
    table.assign(nodes.size(), {});
    for (int32_t node = 0; node < static_cast<int32_t>(nodes.size()); ++node) {
        if (is_leaf(nodes, node))
            continue;
        for (uint32_t b = 0; b < compresskit::BYTE_VALUES; ++b) {
            DecodeEntry& e = table[node][b];
            int32_t cur = node;
            bool corrupt = false;
            for (int bit = compresskit::BITS_PER_BYTE - 1; bit >= 0; --bit) {
                int v = (b >> bit) & 1;
                cur = (v == 0) ? nodes[cur].left : nodes[cur].right;
                if (cur < 0) {
                    // Corrupt stream during table build: shouldn't happen for valid trees.
                    e.count = 0;
                    corrupt = true;
                    break;
                }
                if (is_leaf(nodes, cur)) {
                    e.symbols[e.count++] = nodes[cur].symbol;
                    cur = root;
                }
            }
            e.next = corrupt ? root : cur;
        }
    }
}

}  // namespace

std::vector<uint8_t> huffman_encode_buffer(const std::vector<uint8_t>& input) {
    if (input.size() >= compresskit::MAX_RAW_SIZE) {
        throw std::runtime_error("huffman: input too large");
    }
    std::vector<uint32_t> freq = compresskit::count_frequencies(input);
    freq[compresskit::EOF_SYMBOL] = 1;

    std::vector<Node> nodes;
    nodes.reserve(MAX_TREE_NODES);
    int32_t root = build_tree(freq, nodes);

    std::vector<Code> codes(compresskit::SYMBOL_LIMIT);
    build_codes(nodes, root, codes.data(), 0, 0);

    std::vector<uint8_t> out;
    out.reserve(input.size() + compresskit::INITIAL_ENCODE_OVERHEAD);
    compresskit::write_frequency_header(out, compresskit::HUFFMAN_MAGIC, freq);

    compresskit::BitWriter writer;
    for (uint8_t b : input) {
        writer.write_bits(codes[b].bits, codes[b].len);
    }
    writer.write_bits(codes[compresskit::EOF_SYMBOL].bits, codes[compresskit::EOF_SYMBOL].len);
    std::vector<uint8_t> bits = writer.finish();
    out.insert(out.end(), bits.begin(), bits.end());
    compresskit::append_crc32(out);
    return out;
}

std::vector<uint8_t> huffman_decode_buffer(const std::vector<uint8_t>& input) {
    compresskit::precheck_magic(input, compresskit::HUFFMAN_MAGIC, "huffman");
    std::size_t content = compresskit::verify_crc32(input, "huffman");
    const uint8_t* data = input.data();
    std::size_t pos = 0;
    std::vector<uint32_t> freq = compresskit::read_magic_and_frequency_header(
        data, content, pos, compresskit::HUFFMAN_MAGIC, "huffman");

    if (freq[compresskit::EOF_SYMBOL] == 0) {
        throw std::runtime_error("huffman: frequency table missing EOF symbol");
    }

    std::vector<Node> nodes;
    nodes.reserve(MAX_TREE_NODES);
    int32_t root = build_tree(freq, nodes);

    std::vector<std::array<DecodeEntry, compresskit::BYTE_VALUES>> table;
    build_decode_table(nodes, root, table);

    std::vector<uint8_t> out;
    int32_t cur = root;
    bool saw_eof = false;
    for (std::size_t i = pos; i < content; ++i) {
        const DecodeEntry& e = table[cur][data[i]];
        for (uint8_t k = 0; k < e.count; ++k) {
            uint32_t sym = e.symbols[k];
            if (sym == compresskit::EOF_SYMBOL) {
                saw_eof = true;
                break;
            }
            if (out.size() >= compresskit::MAX_RAW_SIZE) {
                throw compresskit::SizeLimitError("huffman: output size limit exceeded");
            }
            out.push_back(static_cast<uint8_t>(sym));
        }
        if (saw_eof)
            break;
        cur = e.next;
    }
    if (!saw_eof) {
        throw std::runtime_error("huffman: no EOF marker found");
    }
    return out;
}

}  // namespace compresskit

#ifndef COMPRESSKIT_NO_MAIN
#include "compresskit/cli_launcher.hpp"

int main(int argc, char** argv) {
    compresskit::cli::Algorithm algo{compresskit::huffman_encode_buffer,
                                     compresskit::huffman_decode_buffer};
    return compresskit::cli::run(algo, argc, argv);
}
#endif
