#pragma once

#include "workflow/ArtifactResolver.h"

#include <filesystem>
#include <optional>
#include <string_view>

namespace vajra::workflow {

// Selects exactly one Ubuntu 24.04 LTS desktop amd64 image from a publisher
// SHA256SUMS document. Ambiguous or malformed manifests fail closed.
[[nodiscard]] std::optional<ArtifactResolution> resolve_ubuntu_lts_manifest(
    std::string_view manifest);

// Network-first resolution with validated cache and bundled fallback.
[[nodiscard]] std::optional<ArtifactResolution> resolve_ubuntu_lts(
    const std::filesystem::path& cache_path);

} // namespace vajra::workflow
