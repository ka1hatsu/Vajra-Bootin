#include "workflow/UbuntuReleaseService.h"

#include "catalog/ArtifactCache.h"
#include "download/MetadataClient.h"

#include <sstream>
#include <string>
#include <vector>

namespace vajra::workflow {
namespace {
constexpr char kSeriesBase[] = "https://releases.ubuntu.com/24.04/";
constexpr char kManifestUrl[] = "https://releases.ubuntu.com/24.04/SHA256SUMS";

bool candidate_name(const std::string& name) {
    constexpr std::string_view prefix="ubuntu-24.04.";
    constexpr std::string_view suffix="-desktop-amd64.iso";
    return name.starts_with(prefix) && name.ends_with(suffix) &&
           name.find('/')==std::string::npos && name.find('\\')==std::string::npos &&
           name.find("..", prefix.size())==std::string::npos;
}

std::vector<std::string> desktop_names(std::string_view manifest) {
    std::istringstream input{std::string(manifest)}; std::string line; std::vector<std::string> names;
    while(std::getline(input,line)) {
        if(!line.empty()&&line.back()=='\r') line.pop_back();
        if(line.size()<67) continue;
        std::size_t pos=64; while(pos<line.size()&&line[pos]==' ') ++pos; if(pos<line.size()&&line[pos]=='*') ++pos;
        if(pos>=line.size()) continue;
        const std::string name=line.substr(pos); if(candidate_name(name)) names.push_back(name);
    }
    return names;
}
}

std::optional<ArtifactResolution> resolve_ubuntu_lts_manifest(std::string_view manifest) {
    const auto names=desktop_names(manifest); if(names.size()!=1) return std::nullopt;
    const auto& filename=names.front();
    const auto version_end=filename.find("-desktop-amd64.iso");
    if(version_end==std::string::npos||version_end<=7) return std::nullopt;
    catalog::ReleaseArtifact candidate{"ubuntu",filename.substr(7,version_end-7)+" LTS","x86_64",filename,std::string(kSeriesBase)+filename,""};
    return resolve_from_publisher_manifest(std::move(candidate),manifest);
}

std::optional<ArtifactResolution> resolve_ubuntu_lts(const std::filesystem::path& cache_path) {
    if(const auto response=download::fetch_metadata_text(kManifestUrl,512*1024,10000)) {
        if(response->final_url==kManifestUrl) {
            if(auto live=resolve_ubuntu_lts_manifest(response->body)) {
                (void)catalog::write_artifact_cache_atomic(cache_path,live->artifact);
                return live;
            }
        }
    }
    if(const auto cached=catalog::read_artifact_cache(cache_path)) {
        if(cached->distro_id=="ubuntu"&&cached->architecture=="x86_64"&&
           download::check_source_url(cached->download_url).allowed())
            return ArtifactResolution{*cached,ArtifactResolutionSource::BundledFallback};
    }
    return validated_bundled_fallback("ubuntu","x86_64");
}

} // namespace vajra::workflow
