#include "download/SourcePolicy.h"

#include <algorithm>
#include <array>
#include <cctype>

namespace vajra::download {
namespace {

constexpr std::array<std::string_view, 13> trusted_hosts{
    "ubuntu.com",
    "releases.ubuntu.com",
    "cdimage.ubuntu.com",
    "lubuntu.me",
    "cdimage.lubuntu.me",
    "xubuntu.org",
    "cdimage.xubuntu.org",
    "linuxmint.com",
    "mirrors.edge.kernel.org",
    "debian.org",
    "cdimage.debian.org",
    "fedoraproject.org",
    "download.fedoraproject.org"
};

std::string lowercase(std::string_view value) {
    std::string result(value);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return result;
}

bool exact_or_subdomain(std::string_view host, std::string_view trusted) {
    if (host == trusted) return true;
    if (host.size() <= trusted.size()) return false;
    const std::size_t offset = host.size() - trusted.size();
    return host[offset - 1] == '.' && host.substr(offset) == trusted;
}

} // namespace

SourceCheck check_source_url(std::string_view url) {
    constexpr std::string_view https_prefix = "https://";
    if (url.size() < https_prefix.size()) {
        return {SourceDecision::InvalidUrl, {}, "Download URL is incomplete."};
    }

    const std::string lowered = lowercase(url);
    if (lowered.rfind("http://", 0) == 0) {
        return {SourceDecision::InsecureScheme, {}, "Downloads must use HTTPS."};
    }
    if (lowered.rfind(https_prefix, 0) != 0) {
        return {SourceDecision::InvalidUrl, {}, "Only HTTPS download URLs are accepted."};
    }

    const std::size_t authority_start = https_prefix.size();
    const std::size_t authority_end = lowered.find_first_of("/?#", authority_start);
    const std::string authority = lowered.substr(authority_start, authority_end - authority_start);
    if (authority.empty()) {
        return {SourceDecision::InvalidUrl, {}, "Download URL has no host."};
    }
    if (authority.find('@') != std::string::npos) {
        return {SourceDecision::CredentialsNotAllowed, {}, "Credentials in download URLs are not allowed."};
    }

    const std::size_t colon = authority.find(':');
    const std::string host = authority.substr(0, colon);
    if (host.empty()) {
        return {SourceDecision::InvalidUrl, {}, "Download URL has no host."};
    }

    for (const auto trusted : trusted_hosts) {
        if (exact_or_subdomain(host, trusted)) {
            return {SourceDecision::Allowed, host, "Official download source accepted."};
        }
    }

    return {SourceDecision::UntrustedHost, host, "Download host is not in Vajra's trusted-source policy."};
}

} // namespace vajra::download
