#include "catalog/ArtifactCache.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

int main(){
    using namespace vajra::catalog;
    int failures=0; auto check=[&](bool ok,const char* msg){if(!ok){std::cerr<<"FAIL: "<<msg<<'\n';++failures;}};
    const auto dir=std::filesystem::temp_directory_path()/"vajra-artifact-cache-test"; std::error_code ec; std::filesystem::remove_all(dir,ec);
    const auto path=dir/"ubuntu.cache";
    ReleaseArtifact a{"ubuntu","24.04.4 LTS","x86_64","ubuntu.iso","https://releases.ubuntu.com/test/ubuntu.iso",std::string(64,'a')};
    check(write_artifact_cache_atomic(path,a),"valid artifact should be written atomically");
    const auto loaded=read_artifact_cache(path); check(loaded.has_value(),"valid cache should load"); check(loaded&&loaded->sha256==a.sha256,"cache should preserve digest");
    {std::ofstream out(path,std::ios::trunc); out<<"VAJRA_ARTIFACT_V1\nubuntu\n1\nx86_64\n../evil.iso\nhttps://releases.ubuntu.com/evil.iso\n"<<std::string(64,'a')<<'\n';}
    check(!read_artifact_cache(path).has_value(),"tampered cache must fail validation");
    auto invalid=a; invalid.sha256="bad"; check(!write_artifact_cache_atomic(path,invalid),"invalid artifact must not enter cache");
    std::filesystem::remove_all(dir,ec);
    if(failures) return EXIT_FAILURE; std::cout<<"All artifact cache tests passed\n"; return EXIT_SUCCESS;
}
