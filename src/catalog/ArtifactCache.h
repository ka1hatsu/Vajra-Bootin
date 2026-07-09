#pragma once

#include "catalog/ReleaseArtifact.h"

#include <filesystem>
#include <optional>

namespace vajra::catalog {

[[nodiscard]] bool write_artifact_cache_atomic(const std::filesystem::path& path,
                                               const ReleaseArtifact& artifact);
[[nodiscard]] std::optional<ReleaseArtifact> read_artifact_cache(const std::filesystem::path& path);

} // namespace vajra::catalog
