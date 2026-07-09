#include "ui/AppUi.h"

#include <imgui.h>

#include <cstdio>
#include <string>

#include "catalog/Distro.h"
#include "hardware/Scanner.h"
#include "recommender/Recommender.h"

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

void scan_and_open(AppState& state) {
    state.hardware = hardware::scan_hardware();
    state.hardware_scanned = true;
    state.current_screen = Screen::Scan;
}

void render_welcome(AppState& state) {
    ImGui::Dummy(ImVec2(0.0f, 70.0f));
    centered_text("VAJRA");
    centered_text("Bi-Bootin for Windows");
    ImGui::Dummy(ImVec2(0.0f, 14.0f));
    centered_text("Find a Linux distribution suited to your hardware and needs.");
    centered_text("Scan locally, compare compatible choices, download safely, and verify before writing.");
    ImGui::Dummy(ImVec2(0.0f, 34.0f));

    constexpr float button_width = 230.0f;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - button_width) * 0.5f + ImGui::GetCursorPosX());
    if (ImGui::Button("Scan My PC", ImVec2(button_width, 46.0f))) scan_and_open(state);

    ImGui::Dummy(ImVec2(0.0f, 14.0f));
    constexpr float row_width = 470.0f;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - row_width) * 0.5f + ImGui::GetCursorPosX());
    if (ImGui::Button("Choose Linux Myself", ImVec2(225.0f, 38.0f))) state.current_screen = Screen::Recommendations;
    ImGui::SameLine();
    if (ImGui::Button("Download Center", ImVec2(225.0f, 38.0f))) state.current_screen = Screen::Downloads;
}

void profile_row(const char* label, const std::string& value) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("%s", label);
    ImGui::TableSetColumnIndex(1); ImGui::TextWrapped("%s", value.c_str());
}

void render_scan(AppState& state) {
    ImGui::Dummy(ImVec2(0.0f, 28.0f));
    ImGui::TextUnformatted("Hardware detected");
    ImGui::TextDisabled("Read-only local scan. No hardware identifiers are uploaded or stored.");
    ImGui::Separator(); ImGui::Spacing();

    if (!state.hardware_scanned) {
        if (ImGui::Button("Run hardware scan")) scan_and_open(state);
        return;
    }

    const auto& h = state.hardware;
    if (ImGui::BeginTable("HardwareProfile", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 190.0f);
        ImGui::TableSetupColumn("Detected value", ImGuiTableColumnFlags_WidthStretch);
        profile_row("CPU", h.cpu_name);
        profile_row("Architecture", h.architecture);
        profile_row("Logical processors", std::to_string(h.logical_processors));
        profile_row("Physical memory", format_gib(h.memory_bytes));
        profile_row("Graphics adapter", h.gpu_name);
        profile_row("Dedicated GPU memory", format_gib(h.gpu_memory_bytes));
        profile_row("Windows", h.windows_version);
        profile_row("Firmware", h.firmware_mode);
        profile_row("System drive free space", format_gib(h.system_drive_free_bytes));
        ImGui::EndTable();
    }

    ImGui::Dummy(ImVec2(0.0f, 16.0f));
    if (ImGui::Button("Scan again")) state.hardware = hardware::scan_hardware();
    ImGui::SameLine();
    if (ImGui::Button("Back to welcome")) state.current_screen = Screen::Welcome;
    ImGui::SameLine();
    if (ImGui::Button("Find Linux matches")) state.current_screen = Screen::Recommendations;
}

void render_recommendations(AppState& state) {
    ImGui::Dummy(ImVec2(0.0f, 24.0f));
    ImGui::TextUnformatted("Your Linux matches");
    ImGui::TextDisabled("Compatibility is filtered first. Preferences then adjust the ranking.");
    ImGui::Separator(); ImGui::Spacing();

    if (!state.hardware_scanned) {
        ImGui::TextWrapped("Run the hardware scan first so incompatible distributions can be filtered out.");
        if (ImGui::Button("Scan My PC")) scan_and_open(state);
        ImGui::SameLine();
        if (ImGui::Button("Back")) state.current_screen = Screen::Welcome;
        return;
    }

    static const char* purposes[] = {"Daily use", "Coding", "Old PC", "Stable system"};
    static const char* purpose_values[] = {"daily_use", "coding", "old_pc", "stable"};
    static const char* experiences[] = {"Beginner", "Intermediate"};
    static const char* experience_values[] = {"beginner", "intermediate"};

    ImGui::SetNextItemWidth(220.0f);
    ImGui::Combo("Main use", &state.purpose_index, purposes, IM_ARRAYSIZE(purposes));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(220.0f);
    ImGui::Combo("Experience", &state.experience_index, experiences, IM_ARRAYSIZE(experiences));
    ImGui::Spacing();

    recommender::Preferences preferences{purpose_values[state.purpose_index], experience_values[state.experience_index]};
    const auto results = recommender::recommend(state.hardware, catalog::distros(), preferences);

    if (results.empty()) {
        ImGui::TextWrapped("No catalog entry passed the compatibility filters for this hardware profile.");
    }

    for (const auto& result : results) {
        ImGui::PushID(result.distro->id.c_str());
        if (ImGui::BeginChild("card", ImVec2(0.0f, 145.0f), ImGuiChildFlags_Borders)) {
            ImGui::Text("%s", result.distro->name.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("%s | %s", result.distro->desktop.c_str(), result.distro->difficulty.c_str());
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.0f);
            ImGui::Text("%d/100", result.score);
            for (const auto& reason : result.reasons) ImGui::BulletText("%s", reason.c_str());
        }
        ImGui::EndChild();
        ImGui::PopID();
        ImGui::Spacing();
    }

    if (ImGui::Button("Back to hardware")) state.current_screen = Screen::Scan;
    ImGui::SameLine();
    if (ImGui::Button("Home")) state.current_screen = Screen::Welcome;
}

void render_placeholder(AppState& state, const char* title, const char* description) {
    ImGui::Dummy(ImVec2(0.0f, 35.0f));
    ImGui::TextUnformatted(title); ImGui::Separator(); ImGui::Spacing();
    ImGui::TextWrapped("%s", description);
    ImGui::Dummy(ImVec2(0.0f, 18.0f));
    if (ImGui::Button("Back to welcome")) state.current_screen = Screen::Welcome;
}

} // namespace

void apply_theme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f; style.FrameRounding = 7.0f; style.ChildRounding = 8.0f;
    style.PopupRounding = 7.0f; style.ScrollbarRounding = 8.0f; style.GrabRounding = 6.0f;
    style.FramePadding = ImVec2(12.0f, 8.0f); style.ItemSpacing = ImVec2(10.0f, 10.0f);
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.035f, 0.055f, 0.10f, 1.0f);
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.93f, 1.0f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.48f, 0.53f, 0.65f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.29f, 0.55f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.27f, 0.38f, 0.72f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.31f, 0.43f, 0.80f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.07f, 0.10f, 0.17f, 1.0f);
    colors[ImGuiCol_Separator] = ImVec4(0.18f, 0.23f, 0.34f, 1.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.055f, 0.078f, 0.13f, 1.0f);
}

void render(AppState& state) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos); ImGui::SetNextWindowSize(viewport->WorkSize);
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("VajraRoot", nullptr, flags);
    switch (state.current_screen) {
    case Screen::Welcome: render_welcome(state); break;
    case Screen::Scan: render_scan(state); break;
    case Screen::Recommendations: render_recommendations(state); break;
    case Screen::Downloads: render_placeholder(state, "Download Center", "Secure download resolution, resume support, and SHA-256 verification are planned for the download phase."); break;
    default: render_placeholder(state, "Coming soon", "This section is part of a later development phase."); break;
    }
    ImGui::End();
}

} // namespace vajra::ui
