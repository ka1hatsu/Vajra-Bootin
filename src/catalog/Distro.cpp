#include "catalog/Distro.h"

namespace vajra::catalog {

const std::vector<Distro>& distros() {
    static const std::vector<Distro> catalog{
        {"lubuntu", "Lubuntu", 1, 4, 1, {"x86_64"}, "LXQt", "beginner", {"old_pc", "daily_use", "coding"}, "https://lubuntu.me/downloads/", true, true},
        {"linux-mint-xfce", "Linux Mint Xfce", 2, 4, 2, {"x86_64"}, "Xfce", "beginner", {"daily_use", "coding", "beginner"}, "https://www.linuxmint.com/download.php", true, true},
        {"mx-linux-xfce", "MX Linux Xfce", 2, 4, 1, {"x86_64"}, "Xfce", "beginner", {"old_pc", "daily_use"}, "https://mxlinux.org/download-links/", true, true},
        {"xubuntu", "Xubuntu", 2, 4, 2, {"x86_64"}, "Xfce", "beginner", {"daily_use", "coding"}, "https://xubuntu.org/download/", true, true},
        {"fedora-workstation", "Fedora Workstation", 4, 8, 2, {"x86_64"}, "GNOME", "intermediate", {"coding", "daily_use", "modern_pc"}, "https://fedoraproject.org/workstation/download", true, true},
        {"ubuntu", "Ubuntu Desktop", 4, 8, 2, {"x86_64"}, "GNOME", "beginner", {"daily_use", "coding", "beginner"}, "https://ubuntu.com/download/desktop", true, true},
        {"debian-xfce", "Debian Xfce", 2, 4, 1, {"x86_64"}, "Xfce", "intermediate", {"old_pc", "coding", "stable"}, "https://www.debian.org/distrib/", true, true}
    };
    return catalog;
}

} // namespace vajra::catalog
