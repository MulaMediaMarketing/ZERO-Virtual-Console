#pragma once

#include <windows.h>
#include <shlobj.h>
#include <filesystem>

namespace zero {

struct PlatformPaths {
    static std::filesystem::path DataRoot() {
        PWSTR raw = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &raw))) {
            std::filesystem::path root = std::filesystem::path(raw) / L"ZERO";
            CoTaskMemFree(raw);
            return root;
        }
        return std::filesystem::current_path() / L"ZeroData";
    }

    static std::filesystem::path LibraryRoot() { return DataRoot() / L"Library"; }
    static std::filesystem::path SavesRoot() { return DataRoot() / L"Saves"; }
    static std::filesystem::path CacheRoot() { return DataRoot() / L"Cache"; }
    static std::filesystem::path TempRoot() { return DataRoot() / L"Temp"; }
    static std::filesystem::path CapturesRoot() { return DataRoot() / L"Captures"; }
    static std::filesystem::path DataStoreRoot() { return DataRoot() / L"Data"; }
    static std::filesystem::path RuntimeRoot() { return DataRoot() / L"Runtime"; }
    static std::filesystem::path RuntimeSessionsRoot() { return RuntimeRoot() / L"Sessions"; }
    static std::filesystem::path ResumeMirrorRoot() { return DataRoot() / L"Resume"; }
    static std::filesystem::path AchievementsMirrorRoot() { return DataRoot() / L"Achievements"; }
    static std::filesystem::path PlatformStateMirrorRoot() { return DataRoot() / L"PlatformState"; }
    static std::filesystem::path CrashReportsRoot() { return DataRoot() / L"CrashReports"; }
    static std::filesystem::path DiagnosticsRoot() { return DataRoot() / L"Diagnostics"; }
    static std::filesystem::path IdentityRoot() { return DataRoot() / L"Identity"; }
    static std::filesystem::path TrustRoot() { return DataRoot() / L"Trust"; }
    static std::filesystem::path PublisherTrustRoot() { return TrustRoot() / L"Publishers"; }
};

} // namespace zero
