#include "compresskit/cli_launcher.hpp"

#include <cstdio>
#include <iostream>
#include <string>

namespace compresskit {
namespace cli {

namespace {
// program + mode + input + output
constexpr int EXPECTED_ARGC = 4;
const char* kUsage = "Usage: %s encode|decode input output\n";
}  // namespace

int run(const Algorithm& algo, int argc, char** argv) {
    if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        std::fprintf(stderr, kUsage, argv[0]);
        return 0;
    }
    if (argc != EXPECTED_ARGC) {
        std::fprintf(stderr, kUsage, argv[0]);
        return 1;
    }

    std::string mode = argv[1];
    std::string input_path = argv[2];
    std::string output_path = argv[3];

    bool ok = false;
    if (mode == "encode") {
        ok = encode_file_via_buffer(algo.encode, input_path, output_path);
    } else if (mode == "decode") {
        ok = decode_file_via_buffer(algo.decode, input_path, output_path);
    } else {
        std::fprintf(stderr, "unknown mode, expected encode or decode\n");
        std::fprintf(stderr, kUsage, argv[0]);
        return 1;
    }

    return ok ? 0 : 1;
}

}  // namespace cli
}  // namespace compresskit
