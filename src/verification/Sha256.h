#pragma once

#include <filesystem>
#include <string>

namespace vajra::verification {

enum class VerificationState { Verified, Mismatch, InvalidExpectedDigest, FileError, CryptoError };

struct VerificationResult {
    VerificationState state{VerificationState::FileError};
    std::string actual_sha256;
    std::string message;
    [[nodiscard]] bool verified() const noexcept { return state == VerificationState::Verified; }
};

[[nodiscard]] VerificationResult verify_sha256(const std::filesystem::path& path,
                                                const std::string& expected_sha256);

} // namespace vajra::verification
