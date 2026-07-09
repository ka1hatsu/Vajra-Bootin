#include "catalog/ArtifactCache.h"

#include <fstream>
#include <string>
#include <vector>

namespace vajra::catalog {
namespace {
bool safe_field(const std::string& s) { return !s.empty() && s.find('\n')==std::string::npos && s.find('\r')==std::string::npos; }
}

bool write_artifact_cache_atomic(const std::filesystem::path& path,const ReleaseArtifact& a) {
    if(!is_valid_release_artifact(a) || !safe_field(a.distro_id)||!safe_field(a.version)||!safe_field(a.architecture)||!safe_field(a.filename)||!safe_field(a.download_url)||!safe_field(a.sha256)) return false;
    std::error_code ec; if(path.has_parent_path()) std::filesystem::create_directories(path.parent_path(),ec); if(ec) return false;
    const auto temp=path.string()+".tmp";
    { std::ofstream out(temp,std::ios::binary|std::ios::trunc); if(!out) return false;
      out<<"VAJRA_ARTIFACT_V1\n"<<a.distro_id<<'\n'<<a.version<<'\n'<<a.architecture<<'\n'<<a.filename<<'\n'<<a.download_url<<'\n'<<a.sha256<<'\n';
      out.flush(); if(!out) { out.close(); std::filesystem::remove(temp,ec); return false; } }
    std::filesystem::remove(path,ec); ec.clear(); std::filesystem::rename(temp,path,ec); if(ec){ std::filesystem::remove(temp,ec); return false; } return true;
}

std::optional<ReleaseArtifact> read_artifact_cache(const std::filesystem::path& path) {
    std::error_code ec; if(!std::filesystem::exists(path,ec)||ec||std::filesystem::file_size(path,ec)>16*1024||ec) return std::nullopt;
    std::ifstream in(path,std::ios::binary); if(!in) return std::nullopt;
    std::vector<std::string> lines; std::string line; while(std::getline(in,line)){ if(!line.empty()&&line.back()=='\r') line.pop_back(); lines.push_back(line); if(lines.size()>7) return std::nullopt; }
    if(lines.size()!=7||lines[0]!="VAJRA_ARTIFACT_V1") return std::nullopt;
    ReleaseArtifact a{lines[1],lines[2],lines[3],lines[4],lines[5],lines[6]};
    return is_valid_release_artifact(a)?std::optional<ReleaseArtifact>{a}:std::nullopt;
}

} // namespace vajra::catalog
