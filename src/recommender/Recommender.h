#pragma once

#include <string>
#include <vector>

#include "catalog/Distro.h"
#include "hardware/HardwareProfile.h"

namespace vajra::recommender {

struct Preferences {
    std::string purpose{"daily_use"};
    std::string experience{"beginner"};
};

struct Recommendation {
    const catalog::Distro* distro{nullptr};
    int score{0};
    std::vector<std::string> reasons;
};

std::vector<Recommendation> recommend(
    const hardware::HardwareProfile& hardware,
    const std::vector<catalog::Distro>& distros,
    const Preferences& preferences);

} // namespace vajra::recommender
