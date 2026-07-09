#include "catalog/ReleaseArtifact.h"

#include <algorithm>
#include <cctype>

namespace vajra::catalog {
namespace {

bool is_hex_sha256(const std::string& value) noexcept {
    return value.size() == 64 && std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isxdigit(c) != 0;
    });
}

bool has_https_prefix(const std::string& value) noexcept {
    constexpr char prefix[] = "https://";
    return value.size() > sizeof(prefix) - 1 &&
           std::equal(std::begin(prefix), std::end(prefix) - 1, value.begin());
}

} // namespace

bool is_valid_release_artifact(const ReleaseArtifact& artifact) noexcept {
    return !artifact.distro_id.empty() &&
           !artifact.version.empty() &&
           !artifact.architecture.empty() &&
           !artifact.filename.empty() &&
           has_https_prefix(artifact.download_url) &&
           is_hex_sha256(artifact.sha256);
}

const std::vector<ReleaseArtifact>& release_artifacts() {
    // Release-specific URLs and checksums intentionally live here rather than in
    // Distro.cpp. Entries are added only after their URL and digest are verified
    // against the publisher's official release metadata.
    static const std::vector<ReleaseArtifact> artifacts{};
    return artifacts;
}

std::optional<ReleaseArtifact> find_release_artifact(const std::string& distro_id,
                                                     const std::string& architecture) {
    const auto& artifacts = release_artifacts();
    const auto it = std::find_if(artifacts.begin(), artifacts.end(), [&](const ReleaseArtifact& artifact) {
        return artifact.distro_id == distro_id && artifact.architecture == architecture &&
               is_valid_release_artifact(artifact);
    });
    if (it == artifacts.end()) return std::nullopt;
    return *it;
}

} // namespace vajra::catalog
