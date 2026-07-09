#include "workflow/VerifiedDownload.h"

#include <cstdlib>
#include <iostream>

int main() {
    using namespace vajra::workflow;

    VerifiedDownload workflow;
    if (workflow.state() != VerifiedDownloadState::Idle) {
        std::cerr << "FAIL: new workflow should be idle\n";
        return EXIT_FAILURE;
    }

    const bool started = workflow.start(
        "http://releases.ubuntu.com/test.iso",
        "test.iso",
        std::string(64, '0'));

    if (started || workflow.state() != VerifiedDownloadState::Failed || workflow.result().message.empty()) {
        std::cerr << "FAIL: unsafe URL must fail before download\n";
        return EXIT_FAILURE;
    }

    if (workflow.result().ready()) {
        std::cerr << "FAIL: failed workflow must never be ready\n";
        return EXIT_FAILURE;
    }

    std::cout << "All verified download workflow tests passed\n";
    return EXIT_SUCCESS;
}
