#pragma once

#include <d3d11.h>
#include <windows.h>

#include "app/AppState.h"

namespace vajra {

class Application {
public:
    Application() = default;
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    int run(HINSTANCE instance, int show_command);

private:
    bool create_main_window(HINSTANCE instance, int show_command);
    bool create_device();
    void destroy_device();
    void create_render_target();
    void destroy_render_target();
    void render_frame();

    static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    HWND window_{nullptr};
    ID3D11Device* device_{nullptr};
    ID3D11DeviceContext* context_{nullptr};
    IDXGISwapChain* swap_chain_{nullptr};
    ID3D11RenderTargetView* render_target_{nullptr};
    AppState state_{};
};

} // namespace vajra
