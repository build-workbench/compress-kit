#pragma once

#include <cstddef>
#include <stdexcept>

namespace compresskit {

enum class StatusCode {
    OK = 0,
    ERR_CORRUPT,
    ERR_SIZE_LIMIT,
};

// Thrown by codec layers when a size bound (raw data, compressed stream) is
// exceeded, so callers can report ERR_SIZE_LIMIT instead of ERR_CORRUPT.
class SizeLimitError : public std::runtime_error {
public:
    explicit SizeLimitError(const std::string& message) : std::runtime_error(message) {}
};

template <typename T>
struct Result {
    StatusCode status = StatusCode::OK;
    T value{};

    bool ok() const noexcept { return status == StatusCode::OK; }
};

}  // namespace compresskit
