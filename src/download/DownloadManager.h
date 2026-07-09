#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace vajra::download {

enum class DownloadState {
    Idle,
    Running,
    Completed,
    Cancelled,
    Failed
};

struct DownloadProgress {
    std::uint64_t received_bytes{};
    std::uint64_t total_bytes{};
};

using ProgressCallback = std::function<void(const DownloadProgress&)>;
using CompletionCallback = std::function<void(DownloadState, const std::string&)>;

class DownloadManager {
public:
    DownloadManager() = default;
    ~DownloadManager();

    DownloadManager(const DownloadManager&) = delete;
    DownloadManager& operator=(const DownloadManager&) = delete;

    bool start(std::string url, std::filesystem::path destination,
               ProgressCallback progress = {}, CompletionCallback completion = {});
    void cancel() noexcept;
    void wait();

    [[nodiscard]] DownloadState state() const noexcept;
    [[nodiscard]] DownloadProgress progress() const noexcept;
    [[nodiscard]] std::string error() const;

private:
    void run(std::string url, std::filesystem::path destination,
             ProgressCallback progress_callback, CompletionCallback completion_callback);

    std::atomic<DownloadState> state_{DownloadState::Idle};
    std::atomic<bool> cancel_requested_{false};
    std::atomic<std::uint64_t> received_bytes_{0};
    std::atomic<std::uint64_t> total_bytes_{0};
    mutable std::mutex error_mutex_;
    std::string error_;
    std::thread worker_;
};

} // namespace vajra::download
