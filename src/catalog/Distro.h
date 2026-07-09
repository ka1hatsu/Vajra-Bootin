#pragma once

#include <string>
#include <vector>

namespace vajra::catalog {

struct Distro {
    std::string id;
    std::string name;
    double minimum_ram_gib{0.0};
    double recommended_ram_gib{0.0};
    unsigned minimum_processors{1};
    std::vector<std::string> architectures;
    std::string desktop;
    std::string difficulty;
    std::vector<std::string> categories;
    std::string official_download_page;
    bool supports_uefi{true};
    bool supports_legacy_bios{false};
};

const std::vector<Distro>& distros();

} // namespace vajra::catalog
