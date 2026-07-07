#include "ui/AppUi.h"

#include <imgui.h>

namespace vajra::ui {
namespace {

void centered_text(const char* text) {
    const float width = ImGui::CalcTextSize(text).x;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - width) * 0.5f + ImGui::GetCursorPosX());
    ImGui::TextUnformatted(text);
}

void render_welcome(AppState& state) {
    ImGui::Dummy(ImVec2(0.0f, 70.0f));

    ImGui::PushFont(nullptr);
    centered_text("VAJRA");
    centered_text("Bi-Bootin for Windows");
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0.0f, 14.0f));
    centered_text("Find a Linux distribution suited to your hardware and needs.");
    centered_text("Scan locally, compare compatible choices, download safely, and verify before writing.");

    ImGui::Dummy(ImVec2(0.0f, 34.0f));
    const float button_width = 230.0f;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - button_width) * 0.5f + ImGui::GetCursorPosX());
    if (ImGui::Button("Scan My PC", ImVec2(button_width, 46.0f))) {
        state.current_screen = Screen::Scan;
    }

    ImGui::Dummy(ImVec2(0.0f, 14.0f));
    const float row_width = 470.0f;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - row_width) * 0.5f + ImGui::GetCursorPosX());
    if (ImGui::Button("Choose Linux Myself", ImVec2(225.0f, 38.0f))) {
        state.current_screen = Screen::Recommendations;
    }
    ImGui::SameLine();
    if (ImGui::Button("Download Center", ImVec2(225.0f, 38.0f))) {
        state.current_screen = Screen::Downloads;
    }
}

void render_placeholder(AppState& state, const char* title, const char* description) {
    ImGui::Dummy(ImVec2(0.0f, 35.0f));
    ImGui::TextUnformatted(title);
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextWrapped("%s", description);
    ImGui::Dummy(ImVec2(0.0f, 18.0f));
    if (ImGui::Button("Back to welcome")) {
        state.current_screen = Screen::Welcome;
    }
}

} // namespace

void apply_theme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 7.0f;
    style.ChildRounding = 8.0f;
    style.PopupRounding = 7.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 6.0f;
    style.FramePadding = ImVec2(12.0f, 8.0f);
    style.ItemSpacing = ImVec2(10.0f, 10.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.035f, 0.055f, 0.10f, 1.0f);
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.93f, 1.0f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.48f, 0.53f, 0.65f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.29f, 0.55f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.27f, 0.38f, 0.72f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.31f, 0.43f, 0.80f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.07f, 0.10f, 0.17f, 1.0f);
    colors[ImGuiCol_Separator] = ImVec4(0.18f, 0.23f, 0.34f, 1.0f);
}

void render(AppState& state) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("VajraRoot", nullptr, flags);

    switch (state.current_screen) {
    case Screen::Welcome:
        render_welcome(state);
        break;
    case Screen::Scan:
        render_placeholder(state, "Hardware Scan", "Windows hardware discovery will be implemented in Phase W2. The scanner will collect only the information needed for compatibility recommendations.");
        break;
    case Screen::Recommendations:
        render_placeholder(state, "Linux Choices", "The distro catalog and recommendation engine will be ported after the Windows hardware profile is stable.");
        break;
    case Screen::Downloads:
        render_placeholder(state, "Download Center", "Secure download resolution, resume support, and SHA-256 verification are planned after the recommendation layer.");
        break;
    default:
        render_placeholder(state, "Coming soon", "This section is part of a later development phase.");
        break;
    }

    ImGui::End();
}

} // namespace vajra::ui
