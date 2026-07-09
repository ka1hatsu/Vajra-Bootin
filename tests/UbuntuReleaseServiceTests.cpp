#include "workflow/UbuntuReleaseService.h"

#include <cstdlib>
#include <iostream>
#include <string>

int main() {
    using vajra::workflow::resolve_ubuntu_lts_manifest;
    int failures=0; auto check=[&](bool ok,const char* msg){if(!ok){std::cerr<<"FAIL: "<<msg<<'\n';++failures;}};
    const std::string digest(64,'a');
    const std::string good=digest+"  ubuntu-24.04.9-desktop-amd64.iso\n"+std::string(64,'b')+"  ubuntu-24.04.9-live-server-amd64.iso\n";
    const auto resolved=resolve_ubuntu_lts_manifest(good);
    check(resolved.has_value(),"one exact desktop amd64 image should resolve");
    check(resolved&&resolved->artifact.filename=="ubuntu-24.04.9-desktop-amd64.iso","selected filename should be exact");
    check(resolved&&resolved->artifact.version=="24.04.9 LTS","version should derive from selected filename");
    check(resolved&&resolved->artifact.sha256==digest,"publisher digest should be preserved");
    const std::string ambiguous=good+std::string(64,'c')+"  ubuntu-24.04.10-desktop-amd64.iso\n";
    check(!resolve_ubuntu_lts_manifest(ambiguous).has_value(),"multiple desktop candidates must fail closed");
    check(!resolve_ubuntu_lts_manifest(digest+"  ubuntu-25.04-desktop-amd64.iso\n").has_value(),"different Ubuntu series must be rejected");
    check(!resolve_ubuntu_lts_manifest(digest+"  ubuntu-24.04.9-desktop-arm64.iso\n").has_value(),"wrong architecture must be rejected");
    check(!resolve_ubuntu_lts_manifest("bad  ubuntu-24.04.9-desktop-amd64.iso\n").has_value(),"malformed digest must not resolve");
    if(failures) return EXIT_FAILURE; std::cout<<"All Ubuntu release service tests passed\n"; return EXIT_SUCCESS;
}
