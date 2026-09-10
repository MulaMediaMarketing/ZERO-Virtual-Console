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
        f << "ZERO Runtime V3 session launched successfully.\n";
    }

    zero::sdk::Client zero;
    std::wstring error;
    if (!zero.Initialize(error)) {
        MessageBoxW(nullptr, error.c_str(), L"ZERO Runtime V3 SDK Error", MB_OK | MB_ICONERROR);
        return 2;
    }

    if (!zero.ReportReady(error)) {
        MessageBoxW(nullptr, error.c_str(), L"ZERO Runtime V3 READY Error", MB_OK | MB_ICONERROR);
        return 3;
    }

    zero::sdk::ResumeContext resume;
    resume.activityId = "runtime-v3-test";
    resume.displayLabel = "Runtime V3 Validation";
    resume.payload = "checkpoint=validated";
    if (!zero.SetResumeActivity(resume, error)) {
        MessageBoxW(nullptr, error.c_str(), L"ZERO Runtime V3 Resume Error", MB_OK | MB_ICONERROR);
        return 4;
    }

    MessageBoxW(nullptr,
        L"ZERO Runtime V3 validation succeeded.\n\nAuthenticated SDK IPC: OK\nREADY handshake: OK\nResume metadata: OK\n\nClose this window to return to ZERO.",
        L"ZERO Test Game", MB_OK | MB_ICONINFORMATION);
    return 0;
}
