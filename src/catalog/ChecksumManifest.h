#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace vajra::catalog {

// Finds a checksum only for an exact filename match in a publisher checksum manifest.
// Ambiguous duplicate entries fail closed.
[[nodiscard]] std::optional<std::string> sha256_for_exact_filename(
    std::string_view manifest,
    std::string_view filename);

} // namespace vajra::catalog
