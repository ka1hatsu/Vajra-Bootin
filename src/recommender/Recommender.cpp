#include "recommender/Recommender.h"

#include <algorithm>

namespace vajra::recommender {
namespace {

bool contains(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

bool architecture_matches(const std::string& machine, const std::vector<std::string>& supported) {
    if (contains(supported, machine)) return true;
    if (machine == "x86_64") return contains(supported, "amd64");
    if (machine == "amd64") return contains(supported, "x86_64");
    return false;
}

} // namespace

std::vector<Recommendation> recommend(
    const hardware::HardwareProfile& hardware,
    const std::vector<catalog::Distro>& distros,
    const Preferences& preferences) {

    std::vector<Recommendation> results;
    const double ram_gib = static_cast<double>(hardware.memory_bytes) / (1024.0 * 1024.0 * 1024.0);

    for (const auto& distro : distros) {
        if (!architecture_matches(hardware.architecture, distro.architectures)) continue;
        if (ram_gib < distro.minimum_ram_gib) continue;
        if (hardware.logical_processors < distro.minimum_processors) continue;
        if (hardware.firmware_mode == "UEFI" && !distro.supports_uefi) continue;
        if (hardware.firmware_mode == "Legacy BIOS" && !distro.supports_legacy_bios) continue;

        Recommendation result{&distro, 50, {}};

        if (ram_gib >= distro.recommended_ram_gib) {
            result.score += 20;
            result.reasons.emplace_back("Memory meets the recommended level.");
        } else {
            result.score += 10;
            result.reasons.emplace_back("Memory meets the minimum requirement.");
        }

        result.score += 10;
        result.reasons.emplace_back("Processor count is suitable.");

        if ((hardware.firmware_mode == "UEFI" && distro.supports_uefi) ||
            (hardware.firmware_mode == "Legacy BIOS" && distro.supports_legacy_bios)) {
            result.score += 5;
            result.reasons.emplace_back(hardware.firmware_mode + " boot is supported.");
        }

        if (!preferences.purpose.empty() && contains(distro.categories, preferences.purpose)) {
            result.score += 10;
            result.reasons.emplace_back("Matches the selected use case.");
        }

        if (!preferences.experience.empty() && preferences.experience == distro.difficulty) {
            result.score += 5;
            result.reasons.emplace_back("Matches the selected experience level.");
        }

        if (ram_gib <= 4.0 && contains(distro.categories, "old_pc")) {
            result.score += 10;
            result.reasons.emplace_back("Well suited to lower-memory hardware.");
        }

        result.score = std::min(result.score, 100);
        results.push_back(std::move(result));
    }

    std::stable_sort(results.begin(), results.end(), [](const Recommendation& a, const Recommendation& b) {
        return a.score > b.score;
    });
    return results;
}

} // namespace vajra::recommender
