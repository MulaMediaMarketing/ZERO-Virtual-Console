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
        f << "ZERO Console Experience M1 session launched successfully.\n";
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

    std::wstring launchState = L"Play launch: no incoming Resume context.";
    if (zero.LaunchResume()) {
        launchState = L"Resume launch context received from ZERO.";
        if (n > 0) {
            std::ofstream f(std::filesystem::path(savePath) / "resume_launch.txt", std::ios::trunc);
            f << "activity=" << zero.LaunchResume()->activityId << "\n";
            f << "label=" << zero.LaunchResume()->displayLabel << "\n";
            f << "payload=" << zero.LaunchResume()->payload << "\n";
        }
    }

    zero::sdk::ResumeContext resume;
    resume.activityId = "console-m1-checkpoint";
    resume.displayLabel = "Console M1 Validation";
    resume.payload = "checkpoint=console_m1_validated";
    if (!zero.SetResumeActivity(resume, error)) {
        MessageBoxW(nullptr, error.c_str(), L"ZERO Resume Error", MB_OK | MB_ICONERROR);
        return 4;
    }

    if (!zero.UnlockAchievement("console-m1-connected", "Console M1 Connected", error)) {
        MessageBoxW(nullptr, error.c_str(), L"ZERO Achievement Error", MB_OK | MB_ICONERROR);
        return 5;
    }

    std::wstring message = L"ZERO Console Experience M1 validation succeeded.\n\n" + launchState +
        L"\nAuthenticated SDK IPC: OK\nREADY handshake: OK\nResume metadata: OK\nAchievement routing: OK\n\nUse the ZERO overlay while this game is active.";
    MessageBoxW(nullptr, message.c_str(), L"ZERO Test Game", MB_OK | MB_ICONINFORMATION);
    return 0;
}
