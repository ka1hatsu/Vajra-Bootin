#include "workflow/DownloadSelection.h"

#include <cstdlib>
#include <iostream>

int main() {
    using vajra::workflow::prepare_download_selection;

    if (prepare_download_selection("", "Ubuntu", "x86_64").has_value()) {
        std::cerr << "FAIL: empty distro id must be rejected\n";
        return EXIT_FAILURE;
    }
    if (prepare_download_selection("missing", "Missing Linux", "x86_64").has_value()) {
        std::cerr << "FAIL: missing release metadata must not create a download selection\n";
        return EXIT_FAILURE;
    }

    std::cout << "All download selection tests passed\n";
    return EXIT_SUCCESS;
}
