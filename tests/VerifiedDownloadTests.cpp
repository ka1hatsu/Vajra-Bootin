#include "workflow/VerifiedDownload.h"

#include <cstdlib>
#include <iostream>

namespace {
int failures = 0;
void check(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
}

int main() {
    using namespace vajra::workflow;

    {
        VerifiedDownload workflow;
        check(workflow.state() == VerifiedDownloadState::Idle, "new workflow should be idle");
        const bool started = workflow.start("http://releases.ubuntu.com/test.iso", "test.iso", std::string(64, '0'));
        check(!started && workflow.state() == VerifiedDownloadState::Failed,
              "unsafe URL must fail before download");
        check(!workflow.result().message.empty(), "URL rejection should explain the failure");
        check(!workflow.result().ready(), "failed workflow must never be ready");
    }

    {
        VerifiedDownload workflow;
        const bool started = workflow.start("https://releases.ubuntu.com/test.iso", "test.iso", "");
        check(!started && workflow.state() == VerifiedDownloadState::Failed,
              "missing checksum must fail before network activity");
        check(workflow.result().message.find("SHA-256") != std::string::npos,
              "missing checksum failure should explain the requirement");
    }

    if (failures) return EXIT_FAILURE;
    std::cout << "All verified download workflow tests passed\n";
    return EXIT_SUCCESS;
}
