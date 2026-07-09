#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace vajra::download {

struct MetadataResponse {
    std::string body;
    std::string final_url;
};

// Fetches small publisher metadata documents only. The request is bounded by
// time and response size, and every redirect destination is revalidated.
[[nodiscard]] std::optional<MetadataResponse> fetch_metadata_text(
    const std::string& url,
    std::size_t max_bytes = 1024 * 1024,
    int timeout_ms = 10000);

} // namespace vajra::download
