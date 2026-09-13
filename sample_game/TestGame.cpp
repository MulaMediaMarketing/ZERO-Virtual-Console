#include "ZeroSdk.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <string>

int wmain() {
    wchar_t savePath[32768]{};
    DWORD n = GetEnvironmentVariableW(L"ZERO_SAVE_ROOT", savePath, 32768);
    if (n > 0) {
        std::filesystem::path p(savePath);
        std::filesystem::create_directories(p);
        std::ofstream f(p / "test_save.txt", std::ios::app);
        f << "ZERO Runtime V4 hardening session launched successfully.\n";
    }

    zero::sdk::Client zero;
    std::wstring error;
    if (!zero.Initialize(error)) {
        MessageBoxW(nullptr, error.c_str(), L"ZERO SDK Error", MB_OK | MB_ICONERROR);
        return 2;
    }

    if (!zero.ReportReady(error)) {
        MessageBoxW(nullptr, error.c_str(), L"ZERO READY Error", MB_OK | MB_ICONERROR);
        return 3;
    }

    zero::sdk::ResumeContext resume;
    resume.activityId = "runtime-v4-hardening-test";
    resume.displayLabel = "Runtime V4 Validation";
    resume.payload = "checkpoint=v4_hardening_validated";
    if (!zero.SetResumeActivity(resume, error)) {
        MessageBoxW(nullptr, error.c_str(), L"ZERO Resume Error", MB_OK | MB_ICONERROR);
        return 4;
    }

    if (!zero.UnlockAchievement("runtime-v4-connected", "Runtime V4 Connected", error)) {
        MessageBoxW(nullptr, error.c_str(), L"ZERO Achievement Error", MB_OK | MB_ICONERROR);
        return 5;
    }

    MessageBoxW(nullptr,
        L"ZERO Runtime V4 hardening validation succeeded.\n\nAuthenticated SDK IPC: OK\nREADY handshake: OK\nResume metadata: OK\nAchievement routing: OK\n\nClose this window to return to ZERO.",
        L"ZERO Test Game", MB_OK | MB_ICONINFORMATION);
    return 0;
}
