#include "workflow/ArtifactResolver.h"

#include "catalog/ChecksumManifest.h"
#include "download/SourcePolicy.h"

namespace vajra::workflow {

std::optional<ArtifactResolution> resolve_from_publisher_manifest(
    catalog::ReleaseArtifact candidate,
    std::string_view checksum_manifest) {
    const auto source = download::check_source_url(candidate.download_url);
    if (!source.allowed()) return std::nullopt;

    const auto digest = catalog::sha256_for_exact_filename(checksum_manifest, candidate.filename);
    if (!digest.has_value()) return std::nullopt;

    candidate.sha256 = *digest;
    if (!catalog::is_valid_release_artifact(candidate)) return std::nullopt;

    return ArtifactResolution{std::move(candidate), ArtifactResolutionSource::PublisherManifest};
}

std::optional<ArtifactResolution> validated_bundled_fallback(
    const std::string& distro_id,
    const std::string& architecture) {
    const auto artifact = catalog::find_release_artifact(distro_id, architecture);
    if (!artifact.has_value()) return std::nullopt;
    if (!catalog::is_valid_release_artifact(*artifact)) return std::nullopt;
    if (!download::check_source_url(artifact->download_url).allowed()) return std::nullopt;
    return ArtifactResolution{*artifact, ArtifactResolutionSource::BundledFallback};
}

} // namespace vajra::workflow
