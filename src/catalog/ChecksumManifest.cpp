#include "catalog/ChecksumManifest.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace vajra::catalog {
namespace {

bool is_sha256(std::string_view value) {
    return value.size() == 64 && std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isxdigit(c) != 0;
    });
}

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

} // namespace

std::optional<std::string> sha256_for_exact_filename(std::string_view manifest,
                                                     std::string_view filename) {
    if (filename.empty() || filename.find('/') != std::string_view::npos ||
        filename.find('\\') != std::string_view::npos) return std::nullopt;

    std::istringstream input{std::string(manifest)};
    std::string line;
    std::optional<std::string> match;

    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.size() < 67) continue;

        const std::string digest = line.substr(0, 64);
        if (!is_sha256(digest)) continue;

        std::size_t position = 64;
        while (position < line.size() && line[position] == ' ') ++position;
        if (position < line.size() && line[position] == '*') ++position;
        if (position >= line.size()) continue;

        const std::string listed_filename = line.substr(position);
        if (listed_filename != filename) continue;

        const std::string normalized = lowercase(digest);
        if (match.has_value()) return std::nullopt;
        match = normalized;
    }

    return match;
}

} // namespace vajra::catalog
