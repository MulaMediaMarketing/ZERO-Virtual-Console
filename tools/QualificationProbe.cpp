#include <windows.h>
#include <xinput.h>
#include <iostream>
#include <string>

namespace {

struct RTL_OSVERSIONINFOW_LOCAL {
    ULONG dwOSVersionInfoSize;
    ULONG dwMajorVersion;
    ULONG dwMinorVersion;
    ULONG dwBuildNumber;
    ULONG dwPlatformId;
    WCHAR szCSDVersion[128];
};

using RtlGetVersionFn = LONG (WINAPI*)(RTL_OSVERSIONINFOW_LOCAL*);

std::string NativeArchitecture() {
    SYSTEM_INFO info{};
    GetNativeSystemInfo(&info);
    switch (info.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: return "x64";
        case PROCESSOR_ARCHITECTURE_ARM64: return "arm64";
        case PROCESSOR_ARCHITECTURE_INTEL: return "x86";
        default: return "unknown";
    }
}

} // namespace

int wmain() {
    ULONG major = 0, minor = 0, build = 0;
    if (HMODULE ntdll = GetModuleHandleW(L"ntdll.dll")) {
        auto rtlGetVersion = reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion"));
        if (rtlGetVersion) {
            RTL_OSVERSIONINFOW_LOCAL info{};
            info.dwOSVersionInfoSize = sizeof(info);
            if (rtlGetVersion(&info) == 0) {
                major = info.dwMajorVersion;
                minor = info.dwMinorVersion;
                build = info.dwBuildNumber;
            }
        }
    }

    bool controllers[4]{};
    int connectedCount = 0;
    for (DWORD i = 0; i < 4; ++i) {
        XINPUT_STATE state{};
        controllers[i] = XInputGetState(i, &state) == ERROR_SUCCESS;
        if (controllers[i]) ++connectedCount;
    }

    const bool windows11 = major == 10 && build >= 22000;
    const std::string architecture = NativeArchitecture();

    std::cout << "{";
    std::cout << "\"schema\":1,";
    std::cout << "\"os_major\":" << major << ",";
    std::cout << "\"os_minor\":" << minor << ",";
    std::cout << "\"os_build\":" << build << ",";
    std::cout << "\"windows_11\":" << (windows11 ? "true" : "false") << ",";
    std::cout << "\"architecture\":\"" << architecture << "\",";
    std::cout << "\"xinput_connected_count\":" << connectedCount << ",";
    std::cout << "\"xinput_slots\":[";
    for (int i = 0; i < 4; ++i) {
        if (i) std::cout << ',';
        std::cout << (controllers[i] ? "true" : "false");
    }
    std::cout << "]}" << std::endl;
    return 0;
}
