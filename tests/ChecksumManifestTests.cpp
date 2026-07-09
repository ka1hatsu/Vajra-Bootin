#include "catalog/ChecksumManifest.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {
int failures = 0;
void check(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
}

int main() {
    using vajra::catalog::sha256_for_exact_filename;
    const std::string a(64, 'a');
    const std::string b(64, 'B');
    const std::string manifest = a + "  distro-desktop-amd64.iso\n" +
                                 b + " *distro-server-amd64.iso\r\n";

    const auto desktop = sha256_for_exact_filename(manifest, "distro-desktop-amd64.iso");
    check(desktop.has_value() && *desktop == a, "exact desktop filename should resolve");

    const auto server = sha256_for_exact_filename(manifest, "distro-server-amd64.iso");
    check(server.has_value() && *server == std::string(64, 'b'), "binary marker and uppercase digest should parse");

    check(!sha256_for_exact_filename(manifest, "distro.iso").has_value(), "partial filename must not match");
    check(!sha256_for_exact_filename(manifest, "../distro-desktop-amd64.iso").has_value(), "path-like filename must be rejected");
    check(!sha256_for_exact_filename("bad  distro.iso\n", "distro.iso").has_value(), "malformed digest must fail");

    const std::string duplicate = a + "  same.iso\n" + b + "  same.iso\n";
    check(!sha256_for_exact_filename(duplicate, "same.iso").has_value(), "duplicate filename entries must fail closed");

    if (failures) return EXIT_FAILURE;
    std::cout << "All checksum manifest tests passed\n";
    return EXIT_SUCCESS;
}
