# Vajra Bootin for Windows

Vajra Bootin is a Windows desktop application for finding a Linux distribution that suits a computer, downloading its image safely, verifying it, and preparing removable media.

This repository is a native Windows rewrite of the original Vajra Bi-Bootin project. It is being built in C++20 with Dear ImGui, Win32, Direct3D 11, and CMake.

## Current status

The project is at the application-foundation stage. The current code provides the native Win32/D3D11 application shell, Dear ImGui integration, navigation state, and the first welcome screen. Raw disk writing is deliberately not part of this phase.

The planned development order is:

1. application shell and UI foundation;
2. Windows hardware discovery;
3. recommendation engine and distro catalog;
4. secure downloads and SHA-256 verification;
5. conservative removable-drive discovery;
6. a separately elevated, narrowly scoped writer process;
7. packaging, signing, and release automation.

## Building

Requirements:

- Windows 10 or Windows 11
- Visual Studio 2022 with the Desktop development with C++ workload
- CMake 3.24 or newer
- Git

Clone, configure, and build:

```powershell
git clone https://github.com/ka1hatsu/Vajra-Bootin.git
cd Vajra-Bootin
cmake -S . -B build -A x64
cmake --build build --config Release
```

CMake fetches Dear ImGui at configure time. The executable is produced under the build directory for the selected configuration.

## Safety direction

Disk writing will not be added to the GUI process. The planned writer is a separate executable that requests elevation only when needed and accepts a small, validated request. Device identity will be rechecked immediately before destructive operations. System disks and ambiguous targets will be rejected conservatively.

## License

No license has been selected yet. Until one is added, normal copyright rules apply.