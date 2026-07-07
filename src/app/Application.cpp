#include "app/Application.h"

#include <iterator>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "ui/AppUi.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace vajra {
namespace {
constexpr wchar_t kWindowClass[] = L"VajraBootinWindow";
constexpr wchar_t kWindowTitle[] = L"Vajra Bi-Bootin";
}

Application::~Application() { destroy_device(); }

int Application::run(HINSTANCE instance, int show_command) {
    if (!create_main_window(instance, show_command) || !create_device()) {
        MessageBoxW(nullptr, L"Vajra could not initialize the application window or Direct3D 11.", L"Vajra startup error", MB_OK | MB_ICONERROR);
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    ui::apply_theme();
    ImGui_ImplWin32_Init(window_);
    ImGui_ImplDX11_Init(device_, context_);

    MSG message{};
    while (state_.running) {
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
            if (message.message == WM_QUIT) state_.running = false;
        }
        if (!state_.running) break;
        render_frame();
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    destroy_device();
    if (window_ && IsWindow(window_)) DestroyWindow(window_);
    window_ = nullptr;
    UnregisterClassW(kWindowClass, instance);
    return 0;
}

bool Application::create_main_window(HINSTANCE instance, int show_command) {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_CLASSDC;
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.lpszClassName = kWindowClass;
    if (!RegisterClassExW(&window_class)) return false;

    window_ = CreateWindowExW(0, kWindowClass, kWindowTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 720, nullptr, nullptr, instance, this);
    if (!window_) {
        UnregisterClassW(kWindowClass, instance);
        return false;
    }
    ShowWindow(window_, show_command);
    UpdateWindow(window_);
    return true;
}

bool Application::create_device() {
    DXGI_SWAP_CHAIN_DESC descriptor{};
    descriptor.BufferCount = 2;
    descriptor.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    descriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    descriptor.OutputWindow = window_;
    descriptor.SampleDesc.Count = 1;
    descriptor.Windowed = TRUE;
    descriptor.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    constexpr D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL selected_level{};
    const HRESULT result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
        nullptr, 0, levels, static_cast<UINT>(std::size(levels)), D3D11_SDK_VERSION,
        &descriptor, &swap_chain_, &device_, &selected_level, &context_);
    if (FAILED(result)) return false;
    create_render_target();
    return render_target_ != nullptr;
}

void Application::destroy_device() {
    destroy_render_target();
    if (swap_chain_) { swap_chain_->Release(); swap_chain_ = nullptr; }
    if (context_) { context_->Release(); context_ = nullptr; }
    if (device_) { device_->Release(); device_ = nullptr; }
}

void Application::create_render_target() {
    if (!swap_chain_ || !device_) return;
    ID3D11Texture2D* back_buffer = nullptr;
    if (SUCCEEDED(swap_chain_->GetBuffer(0, IID_PPV_ARGS(&back_buffer)))) {
        device_->CreateRenderTargetView(back_buffer, nullptr, &render_target_);
        back_buffer->Release();
    }
}

void Application::destroy_render_target() {
    if (render_target_) { render_target_->Release(); render_target_ = nullptr; }
}

void Application::render_frame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ui::render(state_);
    ImGui::Render();

    constexpr float clear_color[4] = {0.035f, 0.055f, 0.10f, 1.0f};
    context_->OMSetRenderTargets(1, &render_target_, nullptr);
    context_->ClearRenderTargetView(render_target_, clear_color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    swap_chain_->Present(1, 0);
}

LRESULT CALLBACK Application::window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler(hwnd, message, wparam, lparam)) return true;

    Application* application = reinterpret_cast<Application*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
        application = static_cast<Application*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(application));
    }

    switch (message) {
    case WM_SIZE:
        if (application && application->device_ && wparam != SIZE_MINIMIZED) {
            application->destroy_render_target();
            if (SUCCEEDED(application->swap_chain_->ResizeBuffers(0, LOWORD(lparam), HIWORD(lparam), DXGI_FORMAT_UNKNOWN, 0))) {
                application->create_render_target();
            }
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wparam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace vajra
