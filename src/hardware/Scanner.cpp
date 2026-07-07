#include "hardware/Scanner.h"

#include <dxgi1_6.h>
#include <windows.h>

#include <array>
#include <string>

namespace vajra::hardware {
namespace {

std::string wide_to_utf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
}

std::string read_cpu_name() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return "Unknown";
    }

    std::array<wchar_t, 256> buffer{};
    DWORD bytes = static_cast<DWORD>(buffer.size() * sizeof(wchar_t));
    DWORD type = 0;
    const LONG status = RegQueryValueExW(key, L"ProcessorNameString", nullptr, &type, reinterpret_cast<LPBYTE>(buffer.data()), &bytes);
    RegCloseKey(key);

    if (status != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) return "Unknown";
    return wide_to_utf8(buffer.data());
}

std::string architecture_name(WORD architecture) {
    switch (architecture) {
    case PROCESSOR_ARCHITECTURE_AMD64: return "x86_64";
    case PROCESSOR_ARCHITECTURE_ARM64: return "arm64";
    case PROCESSOR_ARCHITECTURE_INTEL: return "x86";
    default: return "Unknown";
    }
}

std::string detect_windows_version() {
    using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
    const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return "Windows";
    const auto rtl_get_version = reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion"));
    if (!rtl_get_version) return "Windows";

    RTL_OSVERSIONINFOW version{};
    version.dwOSVersionInfoSize = sizeof(version);
    if (rtl_get_version(&version) != 0) return "Windows";

    std::string label = version.dwMajorVersion >= 10 && version.dwBuildNumber >= 22000 ? "Windows 11" : "Windows 10";
    label += " (build " + std::to_string(version.dwBuildNumber) + ")";
    return label;
}

std::string detect_firmware_mode() {
    FIRMWARE_TYPE type = FirmwareTypeUnknown;
    if (!GetFirmwareType(&type)) return "Unknown";
    switch (type) {
    case FirmwareTypeUefi: return "UEFI";
    case FirmwareTypeBios: return "Legacy BIOS";
    default: return "Unknown";
    }
}

void detect_gpu(HardwareProfile& profile) {
    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return;

    IDXGIAdapter1* adapter = nullptr;
    for (UINT index = 0; factory->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND; ++index) {
        DXGI_ADAPTER_DESC1 description{};
        if (SUCCEEDED(adapter->GetDesc1(&description)) && !(description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
            profile.gpu_name = wide_to_utf8(description.Description);
            profile.gpu_memory_bytes = static_cast<std::uint64_t>(description.DedicatedVideoMemory);
            adapter->Release();
            factory->Release();
            return;
        }
        adapter->Release();
        adapter = nullptr;
    }
    factory->Release();
}

} // namespace

HardwareProfile scan_hardware() {
    HardwareProfile profile{};
    profile.cpu_name = read_cpu_name();

    SYSTEM_INFO system_info{};
    GetNativeSystemInfo(&system_info);
    profile.architecture = architecture_name(system_info.wProcessorArchitecture);
    profile.logical_processors = system_info.dwNumberOfProcessors;

    MEMORYSTATUSEX memory{};
    memory.dwLength = sizeof(memory);
    if (GlobalMemoryStatusEx(&memory)) profile.memory_bytes = memory.ullTotalPhys;

    profile.windows_version = detect_windows_version();
    profile.firmware_mode = detect_firmware_mode();

    ULARGE_INTEGER free_bytes{};
    if (GetDiskFreeSpaceExW(nullptr, &free_bytes, nullptr, nullptr)) {
        profile.system_drive_free_bytes = free_bytes.QuadPart;
    }

    detect_gpu(profile);
    return profile;
}

} // namespace vajra::hardware
