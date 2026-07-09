#include "download/SourcePolicy.h"

#include <cstdlib>
#include <iostream>

namespace {
int failures = 0;

void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}
}

int main() {
    using vajra::download::SourceDecision;
    using vajra::download::check_source_url;

    check(check_source_url("https://releases.ubuntu.com/24.04/test.iso").allowed(),
          "official Ubuntu HTTPS source should be allowed");
    check(check_source_url("https://cdimage.debian.org/debian-cd/test.iso").allowed(),
          "official Debian HTTPS source should be allowed");
    check(check_source_url("https://download.fedoraproject.org/test.iso").allowed(),
          "official Fedora HTTPS source should be allowed");
    check(check_source_url("HTTP://releases.ubuntu.com/test.iso").decision == SourceDecision::InsecureScheme,
          "HTTP must be rejected case-insensitively");
    check(check_source_url("ftp://releases.ubuntu.com/test.iso").decision == SourceDecision::InvalidUrl,
          "non-HTTPS schemes must be rejected");
    check(check_source_url("https://ubuntu.com.evil.example/test.iso").decision == SourceDecision::UntrustedHost,
          "suffix-confusion host must be rejected");
    check(check_source_url("https://evilubuntu.com/test.iso").decision == SourceDecision::UntrustedHost,
          "lookalike host must be rejected");
    check(check_source_url("https://user@ubuntu.com/test.iso").decision == SourceDecision::CredentialsNotAllowed,
          "userinfo must be rejected");
    check(check_source_url("https://mirror.debian.org/test.iso").allowed(),
          "true subdomain of an allowed official domain should be allowed");
    check(check_source_url("https://example.com/test.iso").decision == SourceDecision::UntrustedHost,
          "unknown host must be rejected");

    if (failures != 0) {
        std::cerr << failures << " source policy test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All source policy tests passed\n";
    return EXIT_SUCCESS;
}
