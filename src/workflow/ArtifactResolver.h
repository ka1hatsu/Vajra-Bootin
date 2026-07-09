#pragma once

#include "catalog/ReleaseArtifact.h"

#include <optional>
#include <string_view>

namespace vajra::workflow {

enum class ArtifactResolutionSource {
    PublisherManifest,
    BundledFallback
};

struct ArtifactResolution {
    catalog::ReleaseArtifact artifact;
    ArtifactResolutionSource source{ArtifactResolutionSource::BundledFallback};
};

// Builds an artifact only when the candidate URL passes source policy and the
// exact ISO filename has one unambiguous SHA-256 entry in publisher metadata.
[[nodiscard]] std::optional<ArtifactResolution> resolve_from_publisher_manifest(
    catalog::ReleaseArtifact candidate,
    std::string_view checksum_manifest);

// Returns a validated bundled artifact when live metadata is unavailable or
// rejected. Invalid fallback entries are never returned.
[[nodiscard]] std::optional<ArtifactResolution> validated_bundled_fallback(
    const std::string& distro_id,
    const std::string& architecture);

} // namespace vajra::workflow
