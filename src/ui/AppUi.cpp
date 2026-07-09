#include "ui/AppUi.h"

#include <imgui.h>

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "catalog/Distro.h"
#include "hardware/Scanner.h"
#include "recommender/Recommender.h"
#include "workflow/DownloadSelection.h"
#include "workflow/VerifiedDownload.h"

namespace vajra::ui {
namespace {

void centered_text(const char* text) {
    const float width = ImGui::CalcTextSize(text).x;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - width) * 0.5f + ImGui::GetCursorPosX());
    ImGui::TextUnformatted(text);
}

std::string format_gib(std::uint64_t bytes) {
    char buffer[64]{};
    std::snprintf(buffer, sizeof(buffer), "%.1f GiB", static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0));
    return buffer;
}

std::string format_bytes(std::uint64_t bytes) {
    static constexpr const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) { value /= 1024.0; ++unit; }
    char buffer[64]{};
    std::snprintf(buffer, sizeof(buffer), "%.1f %s", value, units[unit]);
    return buffer;
}

void copy_text(char* destination, std::size_t capacity, const std::string& value) {
    if (capacity == 0) return;
    std::snprintf(destination, capacity, "%s", value.c_str());
}

void scan_and_open(AppState& state) {
    state.hardware = hardware::scan_hardware();
    state.hardware_scanned = true;
    state.current_screen = Screen::Scan;
}

void render_welcome(AppState& state) {
    ImGui::Dummy(ImVec2(0.0f, 70.0f)); centered_text("VAJRA"); centered_text("Bi-Bootin for Windows");
    ImGui::Dummy(ImVec2(0.0f, 14.0f)); centered_text("Find a Linux distribution suited to your hardware and needs.");
    centered_text("Scan locally, compare compatible choices, download safely, and verify before writing.");
    ImGui::Dummy(ImVec2(0.0f, 34.0f)); constexpr float button_width = 230.0f;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - button_width) * 0.5f + ImGui::GetCursorPosX());
    if (ImGui::Button("Scan My PC", ImVec2(button_width, 46.0f))) scan_and_open(state);
    ImGui::Dummy(ImVec2(0.0f, 14.0f)); constexpr float row_width = 470.0f;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - row_width) * 0.5f + ImGui::GetCursorPosX());
    if (ImGui::Button("Choose Linux Myself", ImVec2(225.0f, 38.0f))) state.current_screen = Screen::Recommendations;
    ImGui::SameLine(); if (ImGui::Button("Download Center", ImVec2(225.0f, 38.0f))) state.current_screen = Screen::Downloads;
}

void profile_row(const char* label, const std::string& value) {
    ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("%s", label);
    ImGui::TableSetColumnIndex(1); ImGui::TextWrapped("%s", value.c_str());
}

void render_scan(AppState& state) {
    ImGui::Dummy(ImVec2(0.0f, 28.0f)); ImGui::TextUnformatted("Hardware detected");
    ImGui::TextDisabled("Read-only local scan. No hardware identifiers are uploaded or stored."); ImGui::Separator(); ImGui::Spacing();
    if (!state.hardware_scanned) { if (ImGui::Button("Run hardware scan")) scan_and_open(state); return; }
    const auto& h = state.hardware;
    if (ImGui::BeginTable("HardwareProfile", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 190.0f); ImGui::TableSetupColumn("Detected value", ImGuiTableColumnFlags_WidthStretch);
        profile_row("CPU", h.cpu_name); profile_row("Architecture", h.architecture); profile_row("Logical processors", std::to_string(h.logical_processors));
        profile_row("Physical memory", format_gib(h.memory_bytes)); profile_row("Graphics adapter", h.gpu_name); profile_row("Dedicated GPU memory", format_gib(h.gpu_memory_bytes));
        profile_row("Windows", h.windows_version); profile_row("Firmware", h.firmware_mode); profile_row("System drive free space", format_gib(h.system_drive_free_bytes)); ImGui::EndTable();
    }
    ImGui::Dummy(ImVec2(0.0f, 16.0f)); if (ImGui::Button("Scan again")) state.hardware = hardware::scan_hardware();
    ImGui::SameLine(); if (ImGui::Button("Back to welcome")) state.current_screen = Screen::Welcome;
    ImGui::SameLine(); if (ImGui::Button("Find Linux matches")) state.current_screen = Screen::Recommendations;
}

void render_recommendations(AppState& state) {
    ImGui::Dummy(ImVec2(0.0f, 24.0f)); ImGui::TextUnformatted("Your Linux matches");
    ImGui::TextDisabled("Compatibility is filtered first. Preferences then adjust the ranking."); ImGui::Separator(); ImGui::Spacing();
    if (!state.hardware_scanned) {
        ImGui::TextWrapped("Run the hardware scan first so incompatible distributions can be filtered out.");
        if (ImGui::Button("Scan My PC")) scan_and_open(state); ImGui::SameLine(); if (ImGui::Button("Back")) state.current_screen = Screen::Welcome; return;
    }
    static const char* purposes[] = {"Daily use", "Coding", "Old PC", "Stable system"};
    static const char* purpose_values[] = {"daily_use", "coding", "old_pc", "stable"};
    static const char* experiences[] = {"Beginner", "Intermediate"}; static const char* experience_values[] = {"beginner", "intermediate"};
    ImGui::SetNextItemWidth(220.0f); ImGui::Combo("Main use", &state.purpose_index, purposes, IM_ARRAYSIZE(purposes));
    ImGui::SameLine(); ImGui::SetNextItemWidth(220.0f); ImGui::Combo("Experience", &state.experience_index, experiences, IM_ARRAYSIZE(experiences)); ImGui::Spacing();
    recommender::Preferences preferences{purpose_values[state.purpose_index], experience_values[state.experience_index]};
    const auto results = recommender::recommend(state.hardware, catalog::distros(), preferences);
    if (results.empty()) ImGui::TextWrapped("No catalog entry passed the compatibility filters for this hardware profile.");
    for (const auto& result : results) {
        ImGui::PushID(result.distro->id.c_str());
        if (ImGui::BeginChild("card", ImVec2(0.0f, 170.0f), ImGuiChildFlags_Borders)) {
            ImGui::Text("%s", result.distro->name.c_str()); ImGui::SameLine(); ImGui::TextDisabled("%s | %s", result.distro->desktop.c_str(), result.distro->difficulty.c_str());
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.0f); ImGui::Text("%d/100", result.score);
            for (const auto& reason : result.reasons) ImGui::BulletText("%s", reason.c_str());
            const auto selection = workflow::prepare_download_selection(result.distro->id, result.distro->name, state.hardware.architecture);
            ImGui::BeginDisabled(!selection.has_value());
            if (ImGui::Button("Download verified image") && selection.has_value()) { state.selected_distro_id = result.distro->id; state.current_screen = Screen::Downloads; }
            ImGui::EndDisabled();
            if (!selection.has_value()) { ImGui::SameLine(); ImGui::TextDisabled("Verified release metadata not available yet"); }
        }
        ImGui::EndChild(); ImGui::PopID(); ImGui::Spacing();
    }
    if (ImGui::Button("Back to hardware")) state.current_screen = Screen::Scan; ImGui::SameLine(); if (ImGui::Button("Home")) state.current_screen = Screen::Welcome;
}

const char* workflow_state_text(workflow::VerifiedDownloadState state) {
    using S = workflow::VerifiedDownloadState;
    switch (state) { case S::Idle: return "Ready"; case S::Downloading: return "Downloading"; case S::Verifying: return "Verifying SHA-256"; case S::Verified: return "Verified and ready"; case S::Cancelled: return "Cancelled"; case S::Failed: return "Failed"; }
    return "Unknown";
}

void render_downloads(AppState& state) {
    static std::unique_ptr<workflow::VerifiedDownload> job = std::make_unique<workflow::VerifiedDownload>();
    static char url[2048]{}; static char destination[1024] = "Downloads\\linux.iso"; static char sha256[65]{}; static std::string loaded_distro;

    if (!state.selected_distro_id.empty() && loaded_distro != state.selected_distro_id) {
        std::string display_name = state.selected_distro_id;
        for (const auto& distro : catalog::distros()) if (distro.id == state.selected_distro_id) { display_name = distro.name; break; }
        const auto selection = workflow::prepare_download_selection(state.selected_distro_id, display_name, state.hardware.architecture);
        if (selection.has_value()) {
            copy_text(url, sizeof(url), selection->artifact.download_url); copy_text(sha256, sizeof(sha256), selection->artifact.sha256);
            copy_text(destination, sizeof(destination), std::string("Downloads\\") + selection->artifact.filename); loaded_distro = state.selected_distro_id;
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 24.0f)); ImGui::TextUnformatted("Download Center");
    ImGui::TextDisabled("HTTPS-only download with source policy checks and SHA-256 verification."); ImGui::Separator(); ImGui::Spacing();
    if (!loaded_distro.empty()) ImGui::Text("Selected release: %s", loaded_distro.c_str());

    const auto current_state = job->state();
    const bool busy = current_state == workflow::VerifiedDownloadState::Downloading || current_state == workflow::VerifiedDownloadState::Verifying;
    ImGui::BeginDisabled(busy); ImGui::SetNextItemWidth(-1.0f); ImGui::InputTextWithHint("##download_url", "Official direct HTTPS ISO URL", url, sizeof(url));
    ImGui::SetNextItemWidth(-1.0f); ImGui::InputTextWithHint("##destination", "Destination path", destination, sizeof(destination));
    ImGui::SetNextItemWidth(-1.0f); ImGui::InputTextWithHint("##sha256", "Expected SHA-256", sha256, sizeof(sha256)); ImGui::EndDisabled(); ImGui::Spacing();

    const auto result = job->result(); const auto download_progress = job->download_progress();
    ImGui::Text("State: %s", workflow_state_text(current_state)); if (!result.message.empty()) ImGui::TextWrapped("%s", result.message.c_str());
    if (!result.sha256.empty()) ImGui::TextWrapped("SHA-256: %s", result.sha256.c_str());
    if (current_state == workflow::VerifiedDownloadState::Downloading) {
        if (download_progress.total_bytes > 0) {
            const float fraction = static_cast<float>(download_progress.received_bytes) / static_cast<float>(download_progress.total_bytes);
            const std::string label = format_bytes(download_progress.received_bytes) + " / " + format_bytes(download_progress.total_bytes);
            ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f), label.c_str());
        } else ImGui::ProgressBar(-1.0f, ImVec2(-1.0f, 0.0f), "Downloading...");
    } else if (current_state == workflow::VerifiedDownloadState::Verifying) ImGui::ProgressBar(-1.0f, ImVec2(-1.0f, 0.0f), "Verifying...");
    else if (current_state == workflow::VerifiedDownloadState::Verified) ImGui::ProgressBar(1.0f, ImVec2(-1.0f, 0.0f), "Verified");
    else ImGui::ProgressBar(0.0f, ImVec2(-1.0f, 0.0f), "Idle");

    ImGui::Spacing();
    if (!busy) {
        if (ImGui::Button("Download and verify")) { if (current_state != workflow::VerifiedDownloadState::Idle) job = std::make_unique<workflow::VerifiedDownload>(); job->start(url, destination, sha256); }
    } else if (ImGui::Button("Cancel download")) job->cancel();
    ImGui::SameLine(); ImGui::BeginDisabled(current_state != workflow::VerifiedDownloadState::Verified);
    if (ImGui::Button("Continue to USB writer")) state.current_screen = Screen::Flash; ImGui::EndDisabled();
    ImGui::SameLine(); if (ImGui::Button("Back to welcome")) state.current_screen = Screen::Welcome;
    ImGui::Spacing(); ImGui::TextDisabled("The USB writer remains unavailable until this workflow reaches Verified state.");
}

void render_placeholder(AppState& state, const char* title, const char* description) {
    ImGui::Dummy(ImVec2(0.0f, 35.0f)); ImGui::TextUnformatted(title); ImGui::Separator(); ImGui::Spacing(); ImGui::TextWrapped("%s", description);
    ImGui::Dummy(ImVec2(0.0f, 18.0f)); if (ImGui::Button("Back to welcome")) state.current_screen = Screen::Welcome;
}

} // namespace

void apply_theme() {
    ImGuiStyle& style = ImGui::GetStyle(); style.WindowRounding = 0.0f; style.FrameRounding = 7.0f; style.ChildRounding = 8.0f; style.PopupRounding = 7.0f; style.ScrollbarRounding = 8.0f; style.GrabRounding = 6.0f;
    style.FramePadding = ImVec2(12.0f, 8.0f); style.ItemSpacing = ImVec2(10.0f, 10.0f); ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.035f, 0.055f, 0.10f, 1.0f); colors[ImGuiCol_Text] = ImVec4(0.90f, 0.93f, 1.0f, 1.0f); colors[ImGuiCol_TextDisabled] = ImVec4(0.48f, 0.53f, 0.65f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.29f, 0.55f, 1.0f); colors[ImGuiCol_ButtonHovered] = ImVec4(0.27f, 0.38f, 0.72f, 1.0f); colors[ImGuiCol_ButtonActive] = ImVec4(0.31f, 0.43f, 0.80f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.07f, 0.10f, 0.17f, 1.0f); colors[ImGuiCol_Separator] = ImVec4(0.18f, 0.23f, 0.34f, 1.0f); colors[ImGuiCol_ChildBg] = ImVec4(0.055f, 0.078f, 0.13f, 1.0f);
}

void render(AppState& state) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport(); ImGui::SetNextWindowPos(viewport->WorkPos); ImGui::SetNextWindowSize(viewport->WorkSize);
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("VajraRoot", nullptr, flags);
    switch (state.current_screen) {
    case Screen::Welcome: render_welcome(state); break; case Screen::Scan: render_scan(state); break; case Screen::Recommendations: render_recommendations(state); break;
    case Screen::Downloads: render_downloads(state); break; case Screen::Flash: render_placeholder(state, "USB Writer", "The verified image handoff is ready. Raw USB writing will be implemented as a separate safety-critical phase."); break;
    default: render_placeholder(state, "Coming soon", "This section is part of a later development phase."); break;
    }
    ImGui::End();
}

} // namespace vajra::ui
