#include "download/DownloadManager.h"
#include "download/SourcePolicy.h"

#include <Windows.h>
#include <winhttp.h>

#include <array>
#include <fstream>
#include <system_error>
#include <utility>

namespace vajra::download {
namespace {

std::wstring utf8_to_wide(const std::string& text) {
    if (text.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                          static_cast<int>(text.size()), nullptr, 0);
    if (count <= 0) return {};
    std::wstring result(static_cast<std::size_t>(count), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                            static_cast<int>(text.size()), result.data(), count) <= 0) {
        return {};
    }
    return result;
}

struct InternetHandle {
    HINTERNET value{};
    ~InternetHandle() { if (value) WinHttpCloseHandle(value); }
    InternetHandle() = default;
    explicit InternetHandle(HINTERNET handle) : value(handle) {}
    InternetHandle(const InternetHandle&) = delete;
    InternetHandle& operator=(const InternetHandle&) = delete;
};

std::string windows_error(const char* operation) {
    return std::string(operation) + " failed with Windows error " + std::to_string(GetLastError());
}

} // namespace

DownloadManager::~DownloadManager() {
    cancel();
    wait();
}

bool DownloadManager::start(std::string url, std::filesystem::path destination,
                            ProgressCallback progress_callback, CompletionCallback completion_callback) {
    if (state_.load() == DownloadState::Running || worker_.joinable()) return false;

    const SourceCheck source = check_source_url(url);
    if (!source.allowed()) {
        std::lock_guard lock(error_mutex_);
        error_ = source.message;
        state_.store(DownloadState::Failed);
        return false;
    }

    cancel_requested_.store(false);
    received_bytes_.store(0);
    total_bytes_.store(0);
    {
        std::lock_guard lock(error_mutex_);
        error_.clear();
    }
    state_.store(DownloadState::Running);
    worker_ = std::thread(&DownloadManager::run, this, std::move(url), std::move(destination),
                          std::move(progress_callback), std::move(completion_callback));
    return true;
}

void DownloadManager::cancel() noexcept {
    cancel_requested_.store(true);
}

void DownloadManager::wait() {
    if (worker_.joinable()) worker_.join();
}

DownloadState DownloadManager::state() const noexcept {
    return state_.load();
}

DownloadProgress DownloadManager::progress() const noexcept {
    return {received_bytes_.load(), total_bytes_.load()};
}

std::string DownloadManager::error() const {
    std::lock_guard lock(error_mutex_);
    return error_;
}

void DownloadManager::run(std::string url, std::filesystem::path destination,
                          ProgressCallback progress_callback, CompletionCallback completion_callback) {
    const auto finish = [&](DownloadState final_state, std::string message) {
        if (final_state == DownloadState::Failed) {
            std::lock_guard lock(error_mutex_);
            error_ = message;
        }
        state_.store(final_state);
        if (completion_callback) completion_callback(final_state, message);
    };

    const std::filesystem::path partial = destination.string() + ".part";
    const std::wstring wide_url = utf8_to_wide(url);
    if (wide_url.empty()) {
        finish(DownloadState::Failed, "Download URL is not valid UTF-8.");
        return;
    }

    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwSchemeLength = static_cast<DWORD>(-1);
    parts.dwHostNameLength = static_cast<DWORD>(-1);
    parts.dwUrlPathLength = static_cast<DWORD>(-1);
    parts.dwExtraInfoLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(wide_url.c_str(), 0, 0, &parts)) {
        finish(DownloadState::Failed, windows_error("URL parsing"));
        return;
    }

    const std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
    std::wstring resource(parts.lpszUrlPath, parts.dwUrlPathLength);
    if (parts.dwExtraInfoLength) resource.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);

    InternetHandle session(WinHttpOpen(L"Vajra-Bootin/0.4", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session.value) { finish(DownloadState::Failed, windows_error("WinHttpOpen")); return; }

    InternetHandle connection(WinHttpConnect(session.value, host.c_str(), parts.nPort, 0));
    if (!connection.value) { finish(DownloadState::Failed, windows_error("WinHttpConnect")); return; }

    InternetHandle request(WinHttpOpenRequest(connection.value, L"GET", resource.c_str(), nullptr,
                                               WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                               WINHTTP_FLAG_SECURE));
    if (!request.value) { finish(DownloadState::Failed, windows_error("WinHttpOpenRequest")); return; }

    DWORD redirect_policy = WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
    WinHttpSetOption(request.value, WINHTTP_OPTION_REDIRECT_POLICY, &redirect_policy, sizeof(redirect_policy));

    if (!WinHttpSendRequest(request.value, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request.value, nullptr)) {
        finish(DownloadState::Failed, windows_error("HTTP request"));
        return;
    }

    DWORD status = 0;
    DWORD status_size = sizeof(status);
    if (!WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX) ||
        status < 200 || status >= 300) {
        finish(DownloadState::Failed, "Server returned HTTP status " + std::to_string(status) + ".");
        return;
    }

    std::uint64_t content_length = 0;
    wchar_t length_buffer[64]{};
    DWORD length_size = sizeof(length_buffer);
    if (WinHttpQueryHeaders(request.value, WINHTTP_QUERY_CONTENT_LENGTH,
                            WINHTTP_HEADER_NAME_BY_INDEX, length_buffer, &length_size,
                            WINHTTP_NO_HEADER_INDEX)) {
        try { content_length = std::stoull(length_buffer); } catch (...) { content_length = 0; }
    }
    total_bytes_.store(content_length);

    std::error_code ec;
    if (destination.has_parent_path()) std::filesystem::create_directories(destination.parent_path(), ec);
    if (ec) { finish(DownloadState::Failed, "Could not create destination directory: " + ec.message()); return; }

    std::ofstream output(partial, std::ios::binary | std::ios::trunc);
    if (!output) { finish(DownloadState::Failed, "Could not open temporary download file."); return; }

    std::array<char, 256 * 1024> buffer{};
    std::uint64_t received = 0;
    for (;;) {
        if (cancel_requested_.load()) {
            output.close();
            finish(DownloadState::Cancelled, "Download cancelled; partial file kept for future resume support.");
            return;
        }

        DWORD bytes_read = 0;
        if (!WinHttpReadData(request.value, buffer.data(), static_cast<DWORD>(buffer.size()), &bytes_read)) {
            output.close();
            finish(DownloadState::Failed, windows_error("Download read"));
            return;
        }
        if (bytes_read == 0) break;

        output.write(buffer.data(), static_cast<std::streamsize>(bytes_read));
        if (!output) {
            output.close();
            finish(DownloadState::Failed, "Writing the temporary download file failed.");
            return;
        }

        received += bytes_read;
        received_bytes_.store(received);
        if (progress_callback) progress_callback({received, content_length});
    }

    output.close();
    if (content_length != 0 && received != content_length) {
        finish(DownloadState::Failed, "Downloaded size does not match Content-Length.");
        return;
    }

    std::filesystem::remove(destination, ec);
    ec.clear();
    std::filesystem::rename(partial, destination, ec);
    if (ec) {
        finish(DownloadState::Failed, "Could not finalize downloaded file: " + ec.message());
        return;
    }

    finish(DownloadState::Completed, "Download completed.");
}

} // namespace vajra::download
