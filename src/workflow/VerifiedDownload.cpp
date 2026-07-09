#include "workflow/VerifiedDownload.h"

#include <utility>

namespace vajra::workflow {

VerifiedDownload::~VerifiedDownload() {
    cancel();
    wait();
}

bool VerifiedDownload::start(std::string url,
                             std::filesystem::path destination,
                             std::string expected_sha256,
                             WorkflowProgressCallback progress,
                             WorkflowStateCallback state_changed,
                             WorkflowCompletionCallback completion) {
    if (state_.load() == VerifiedDownloadState::Downloading ||
        state_.load() == VerifiedDownloadState::Verifying) {
        return false;
    }

    downloader_.wait();
    state_.store(VerifiedDownloadState::Downloading);
    set_result({VerifiedDownloadState::Downloading, destination, {}, "Download started."});
    if (state_changed) state_changed(VerifiedDownloadState::Downloading);

    const auto path_for_callback = destination;
    const bool started = downloader_.start(
        std::move(url), std::move(destination), std::move(progress),
        [this, path_for_callback, expected_sha256 = std::move(expected_sha256),
         state_changed = std::move(state_changed), completion = std::move(completion)]
        (download::DownloadState download_state, const std::string& download_message) mutable {
            if (download_state == download::DownloadState::Cancelled) {
                state_.store(VerifiedDownloadState::Cancelled);
                VerifiedDownloadResult final_result{VerifiedDownloadState::Cancelled, path_for_callback, {}, download_message};
                set_result(final_result);
                if (state_changed) state_changed(VerifiedDownloadState::Cancelled);
                if (completion) completion(final_result);
                return;
            }

            if (download_state != download::DownloadState::Completed) {
                state_.store(VerifiedDownloadState::Failed);
                VerifiedDownloadResult final_result{VerifiedDownloadState::Failed, path_for_callback, {}, download_message};
                set_result(final_result);
                if (state_changed) state_changed(VerifiedDownloadState::Failed);
                if (completion) completion(final_result);
                return;
            }

            state_.store(VerifiedDownloadState::Verifying);
            set_result({VerifiedDownloadState::Verifying, path_for_callback, {}, "Download complete; verifying SHA-256."});
            if (state_changed) state_changed(VerifiedDownloadState::Verifying);

            const verification::VerificationResult verification =
                verification::verify_sha256(path_for_callback, expected_sha256);

            if (!verification.verified()) {
                state_.store(VerifiedDownloadState::Failed);
                VerifiedDownloadResult final_result{
                    VerifiedDownloadState::Failed,
                    path_for_callback,
                    verification.actual_sha256,
                    verification.message
                };
                set_result(final_result);
                if (state_changed) state_changed(VerifiedDownloadState::Failed);
                if (completion) completion(final_result);
                return;
            }

            state_.store(VerifiedDownloadState::Verified);
            VerifiedDownloadResult final_result{
                VerifiedDownloadState::Verified,
                path_for_callback,
                verification.actual_sha256,
                "Download completed and SHA-256 verified."
            };
            set_result(final_result);
            if (state_changed) state_changed(VerifiedDownloadState::Verified);
            if (completion) completion(final_result);
        });

    if (!started) {
        state_.store(VerifiedDownloadState::Failed);
        set_result({VerifiedDownloadState::Failed, path_for_callback, {}, downloader_.error()});
        if (state_changed) state_changed(VerifiedDownloadState::Failed);
        return false;
    }

    return true;
}

void VerifiedDownload::cancel() noexcept {
    downloader_.cancel();
}

void VerifiedDownload::wait() {
    downloader_.wait();
}

VerifiedDownloadState VerifiedDownload::state() const noexcept {
    return state_.load();
}

VerifiedDownloadResult VerifiedDownload::result() const {
    std::lock_guard lock(result_mutex_);
    return result_;
}

void VerifiedDownload::set_result(VerifiedDownloadResult result) {
    std::lock_guard lock(result_mutex_);
    result_ = std::move(result);
}

} // namespace vajra::workflow
