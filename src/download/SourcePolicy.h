#pragma once

#include <string>
#include <string_view>

namespace vajra::download {

enum class SourceDecision {
    Allowed,
    InvalidUrl,
    InsecureScheme,
    UntrustedHost,
    CredentialsNotAllowed
};

struct SourceCheck {
    SourceDecision decision{SourceDecision::InvalidUrl};
    std::string host;
    std::string message;

    [[nodiscard]] bool allowed() const noexcept {
        return decision == SourceDecision::Allowed;
    }
};

[[nodiscard]] SourceCheck check_source_url(std::string_view url);

} // namespace vajra::download
