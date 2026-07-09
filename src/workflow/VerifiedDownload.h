#pragma once

#include "download/DownloadManager.h"
#include "verification/Sha256.h"

#include <atomic>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>

namespace vajra::workflow {

enum class VerifiedDownloadState { Idle, Downloading, Verifying, Verified, Cancelled, Failed };

struct VerifiedDownloadResult {
    VerifiedDownloadState state{VerifiedDownloadState::Idle};
    std::filesystem::path path;
    std::string sha256;
    std::string message;
    [[nodiscard]] bool ready() const noexcept { return state == VerifiedDownloadState::Verified; }
};

using WorkflowProgressCallback = std::function<void(const download::DownloadProgress&)>;
using WorkflowStateCallback = std::function<void(VerifiedDownloadState)>;
using WorkflowCompletionCallback = std::function<void(const VerifiedDownloadResult&)>;

class VerifiedDownload {
public:
    VerifiedDownload() = default;
    ~VerifiedDownload();
    VerifiedDownload(const VerifiedDownload&) = delete;
    VerifiedDownload& operator=(const VerifiedDownload&) = delete;

    bool start(std::string url, std::filesystem::path destination, std::string expected_sha256,
               WorkflowProgressCallback progress = {}, WorkflowStateCallback state_changed = {},
               WorkflowCompletionCallback completion = {});
    void cancel() noexcept;
    void wait();
    [[nodiscard]] VerifiedDownloadState state() const noexcept;
    [[nodiscard]] download::DownloadProgress download_progress() const noexcept;
    [[nodiscard]] VerifiedDownloadResult result() const;

private:
    void set_result(VerifiedDownloadResult result);
    download::DownloadManager downloader_;
    std::atomic<VerifiedDownloadState> state_{VerifiedDownloadState::Idle};
    mutable std::mutex result_mutex_;
    VerifiedDownloadResult result_;
};

} // namespace vajra::workflow
