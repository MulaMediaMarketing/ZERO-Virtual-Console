#include <windows.h>
#include <xinput.h>
#include <iostream>
#include <sstream>
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

std::string JsonEscape(const std::string& text) {
    std::ostringstream out;
    for (unsigned char c : text) {
        switch (c) {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20) {
                    const char* hex = "0123456789abcdef";
                    out << "\\u00" << hex[(c >> 4) & 0xf] << hex[c & 0xf];
                } else {
                    out << static_cast<char>(c);
                }
        }
    }
    return out.str();
}

std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string out(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), out.data(), size, nullptr, nullptr);
    return out;
}

std::string MachineName() {
    wchar_t buffer[MAX_COMPUTERNAME_LENGTH + 1]{};
    DWORD size = static_cast<DWORD>(std::size(buffer));
    if (!GetComputerNameW(buffer, &size)) return {};
    return WideToUtf8(std::wstring(buffer, size));
}

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
    std::cout << "\"machine_name\":\"" << JsonEscape(MachineName()) << "\",";
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
