#pragma once

#include "catalog/ReleaseArtifact.h"

#include <optional>
#include <string>

namespace vajra::workflow {

struct DownloadSelection {
    std::string distro_id;
    std::string display_name;
    catalog::ReleaseArtifact artifact;
};

[[nodiscard]] std::optional<DownloadSelection> prepare_download_selection(
    const std::string& distro_id,
    const std::string& display_name,
    const std::string& architecture);

} // namespace vajra::workflow
