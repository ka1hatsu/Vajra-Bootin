#include "workflow/ArtifactResolver.h"

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
    using namespace vajra;
    const std::string digest(64, 'a');
    catalog::ReleaseArtifact candidate{
        "ubuntu", "test", "x86_64", "ubuntu-test-desktop-amd64.iso",
        "https://releases.ubuntu.com/test/ubuntu-test-desktop-amd64.iso", ""
    };

    const auto resolved = workflow::resolve_from_publisher_manifest(
        candidate, digest + "  ubuntu-test-desktop-amd64.iso\n");
    check(resolved.has_value(), "trusted URL plus exact publisher manifest entry should resolve");
    check(resolved && resolved->artifact.sha256 == digest, "resolved artifact should carry manifest digest");
    check(resolved && resolved->source == workflow::ArtifactResolutionSource::PublisherManifest,
          "live resolution source should be recorded");

    check(!workflow::resolve_from_publisher_manifest(
        candidate, digest + "  ubuntu-test-server-amd64.iso\n").has_value(),
        "wrong edition filename must not resolve");

    auto untrusted = candidate;
    untrusted.download_url = "https://example.com/ubuntu-test-desktop-amd64.iso";
    check(!workflow::resolve_from_publisher_manifest(
        untrusted, digest + "  ubuntu-test-desktop-amd64.iso\n").has_value(),
        "untrusted download host must not resolve even with a matching manifest");

    const auto fallback = workflow::validated_bundled_fallback("ubuntu", "x86_64");
    check(fallback.has_value(), "verified Ubuntu fallback should be available");
    check(fallback && fallback->source == workflow::ArtifactResolutionSource::BundledFallback,
          "fallback source should be recorded");
    check(!workflow::validated_bundled_fallback("missing", "x86_64").has_value(),
          "unknown fallback must fail closed");

    if (failures) return EXIT_FAILURE;
    std::cout << "All artifact resolver tests passed\n";
    return EXIT_SUCCESS;
}
