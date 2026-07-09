#include "workflow/DownloadSelection.h"

namespace vajra::workflow {

std::optional<DownloadSelection> prepare_download_selection(const std::string& distro_id,
                                                            const std::string& display_name,
                                                            const std::string& architecture) {
    if (distro_id.empty() || display_name.empty() || architecture.empty()) return std::nullopt;
    const auto artifact = catalog::find_release_artifact(distro_id, architecture);
    if (!artifact.has_value()) return std::nullopt;
    return DownloadSelection{distro_id, display_name, *artifact};
}

} // namespace vajra::workflow
