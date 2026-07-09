#include "verification/Sha256.h"

#include <Windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <vector>

namespace vajra::verification {
namespace {

bool valid_digest(const std::string& digest) {
    return digest.size() == 64 && std::all_of(digest.begin(), digest.end(), [](unsigned char c) {
        return std::isxdigit(c) != 0;
    });
}

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

struct AlgorithmGuard {
    BCRYPT_ALG_HANDLE handle{};
    ~AlgorithmGuard() { if (handle) BCryptCloseAlgorithmProvider(handle, 0); }
};

struct HashGuard {
    BCRYPT_HASH_HANDLE handle{};
    ~HashGuard() { if (handle) BCryptDestroyHash(handle); }
};

} // namespace

VerificationResult verify_sha256(const std::filesystem::path& path,
                                 const std::string& expected_sha256) {
    if (!valid_digest(expected_sha256)) {
        return {VerificationState::InvalidExpectedDigest, {}, "Expected SHA-256 must contain exactly 64 hexadecimal characters."};
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) return {VerificationState::FileError, {}, "Could not open file for verification."};

    AlgorithmGuard algorithm;
    if (BCryptOpenAlgorithmProvider(&algorithm.handle, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) {
        return {VerificationState::CryptoError, {}, "Could not initialize Windows SHA-256 provider."};
    }

    DWORD object_length = 0;
    DWORD result_size = 0;
    if (BCryptGetProperty(algorithm.handle, BCRYPT_OBJECT_LENGTH,
                          reinterpret_cast<PUCHAR>(&object_length), sizeof(object_length),
                          &result_size, 0) < 0) {
        return {VerificationState::CryptoError, {}, "Could not query SHA-256 provider."};
    }

    std::vector<UCHAR> hash_object(object_length);
    HashGuard hash;
    if (BCryptCreateHash(algorithm.handle, &hash.handle, hash_object.data(), object_length,
                         nullptr, 0, 0) < 0) {
        return {VerificationState::CryptoError, {}, "Could not create SHA-256 hash state."};
    }

    std::array<char, 1024 * 1024> buffer{};
    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize count = input.gcount();
        if (count > 0 && BCryptHashData(hash.handle, reinterpret_cast<PUCHAR>(buffer.data()),
                                        static_cast<ULONG>(count), 0) < 0) {
            return {VerificationState::CryptoError, {}, "SHA-256 hashing failed."};
        }
    }
    if (!input.eof()) return {VerificationState::FileError, {}, "Reading file during verification failed."};

    std::array<UCHAR, 32> digest{};
    if (BCryptFinishHash(hash.handle, digest.data(), static_cast<ULONG>(digest.size()), 0) < 0) {
        return {VerificationState::CryptoError, {}, "Could not finalize SHA-256 hash."};
    }

    std::ostringstream hex;
    hex << std::hex << std::setfill('0');
    for (const UCHAR byte : digest) hex << std::setw(2) << static_cast<unsigned int>(byte);
    const std::string actual = hex.str();
    const std::string expected = lowercase(expected_sha256);

    if (actual != expected) {
        return {VerificationState::Mismatch, actual, "SHA-256 mismatch. The file must not be written to USB."};
    }
    return {VerificationState::Verified, actual, "SHA-256 verified."};
}

} // namespace vajra::verification
