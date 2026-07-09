#include "download/DownloadManager.h"

#include <cstdlib>
#include <iostream>

namespace {
int failures = 0;
void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}
}

int main() {
    using vajra::download::DownloadManager;
    using vajra::download::DownloadState;

    DownloadManager manager;
    check(manager.state() == DownloadState::Idle, "new manager should be idle");
    check(!manager.start("http://releases.ubuntu.com/test.iso", "test.iso"),
          "insecure source must be rejected before worker creation");
    check(manager.state() == DownloadState::Failed, "rejected source should set failed state");
    check(!manager.error().empty(), "rejected source should expose an error message");

    DownloadManager unknown;
    check(!unknown.start("https://example.com/test.iso", "test.iso"),
          "untrusted source must be rejected before worker creation");
    check(unknown.state() == DownloadState::Failed, "untrusted source should set failed state");

    if (failures != 0) {
        std::cerr << failures << " download manager test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All download manager tests passed\n";
    return EXIT_SUCCESS;
}
