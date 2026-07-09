#include "verification/Sha256.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {
int failures = 0;
void check(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
}

int main() {
    using namespace vajra::verification;
    const auto path = std::filesystem::temp_directory_path() / "vajra_sha256_test.txt";
    { std::ofstream output(path, std::ios::binary); output << "abc"; }

    const std::string known = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    check(verify_sha256(path, known).verified(), "known SHA-256 vector should verify");
    check(verify_sha256(path, "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD").verified(),
          "uppercase expected digest should verify");
    check(verify_sha256(path, std::string(64, '0')).state == VerificationState::Mismatch,
          "wrong digest must report mismatch");
    check(verify_sha256(path, "bad").state == VerificationState::InvalidExpectedDigest,
          "malformed expected digest must be rejected");
    check(verify_sha256(path.string() + ".missing", known).state == VerificationState::FileError,
          "missing file must report file error");

    std::error_code ec;
    std::filesystem::remove(path, ec);
    if (failures) return EXIT_FAILURE;
    std::cout << "All SHA-256 tests passed\n";
    return EXIT_SUCCESS;
}
