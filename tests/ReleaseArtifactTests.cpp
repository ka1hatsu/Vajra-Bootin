#include "catalog/ReleaseArtifact.h"

#include <cstdlib>
#include <iostream>

namespace {
int failures = 0;
void check(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
}

int main() {
    using namespace vajra::catalog;

    ReleaseArtifact valid{
        "example", "1.0", "x86_64", "example.iso",
        "https://downloads.example.org/example.iso", std::string(64, 'a')
    };
    check(is_valid_release_artifact(valid), "complete HTTPS artifact should validate");

    auto bad_url = valid;
    bad_url.download_url = "http://downloads.example.org/example.iso";
    check(!is_valid_release_artifact(bad_url), "HTTP artifact must be rejected");

    auto bad_digest = valid;
    bad_digest.sha256 = "not-a-digest";
    check(!is_valid_release_artifact(bad_digest), "malformed digest must be rejected");

    auto missing_filename = valid;
    missing_filename.filename.clear();
    check(!is_valid_release_artifact(missing_filename), "missing filename must be rejected");

    check(!find_release_artifact("missing", "x86_64").has_value(),
          "unknown distro must not produce fabricated metadata");

    for (const auto& artifact : release_artifacts()) {
        check(is_valid_release_artifact(artifact), "every bundled artifact must pass validation");
    }

    if (failures) return EXIT_FAILURE;
    std::cout << "All release artifact catalog tests passed\n";
    return EXIT_SUCCESS;
}
