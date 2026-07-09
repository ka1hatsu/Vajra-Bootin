#pragma once

#include <optional>
#include <string>
#include <vector>

namespace vajra::catalog {

struct ReleaseArtifact {
    std::string distro_id;
    std::string version;
    std::string architecture;
    std::string filename;
    std::string download_url;
    std::string sha256;
};

[[nodiscard]] bool is_valid_release_artifact(const ReleaseArtifact& artifact) noexcept;
[[nodiscard]] const std::vector<ReleaseArtifact>& release_artifacts();
[[nodiscard]] std::optional<ReleaseArtifact> find_release_artifact(const std::string& distro_id,
                                                                   const std::string& architecture);

} // namespace vajra::catalog
