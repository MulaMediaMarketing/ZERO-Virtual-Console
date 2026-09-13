#include "ZeroSdk.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
std::filesystem::path envPath(const wchar_t* key) {
    wchar_t buffer[32768]{};
    const DWORD n = GetEnvironmentVariableW(key, buffer, static_cast<DWORD>(std::size(buffer)));
    if (!n || n >= std::size(buffer)) return {};
    return std::filesystem::path(buffer);
}

void writeMarker(const std::filesystem::path& root, const char* name, const std::string& body) {
    if (root.empty()) return;
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    std::ofstream f(root / name, std::ios::binary | std::ios::trunc);
    if (f) f << body;
}
}

int wmain() {
    const auto saveRoot = envPath(L"ZERO_SAVE_ROOT");
    writeMarker(saveRoot, "launch.ok", "reference_game_launched=1\n");

    zero::sdk::Client sdk;
    std::wstring error;
    if (!sdk.Initialize(error)) return 20;
    if (!sdk.ReportReady(error)) return 21;
    writeMarker(saveRoot, "ready.ok", "sdk_ready=1\n");

    if (const auto resume = sdk.LaunchResume()) {
        const bool valid = resume->activityId == "m1-reference-checkpoint" &&
                           resume->payload == "checkpoint=m1_reference_validated";
        std::string body = std::string("resume_valid=") + (valid ? "1" : "0") + "\n" +
            "activity=" + resume->activityId + "\n" +
            "label=" + resume->displayLabel + "\n" +
            "payload=" + resume->payload + "\n";
        writeMarker(saveRoot, "resume_roundtrip.ok", body);
        if (!valid) return 22;
    }

    zero::sdk::ResumeContext resume;
    resume.activityId = "m1-reference-checkpoint";
    resume.displayLabel = "ZERO Reference Checkpoint";
    resume.payload = "checkpoint=m1_reference_validated";
    if (!sdk.SetResumeActivity(resume, error)) return 23;

    if (!sdk.UnlockAchievement("m1-reference-connected", "Reference Experience Connected", error)) return 24;
    writeMarker(saveRoot, "sdk_contract.ok", "resume_set=1\nachievement_set=1\n");

    const auto crashSentinel = saveRoot / "force_crash.next";
    std::error_code ec;
    if (!saveRoot.empty() && std::filesystem::exists(crashSentinel, ec)) {
        std::filesystem::remove(crashSentinel, ec);
        writeMarker(saveRoot, "intentional_crash.triggered", "exit_code=73\n");
        return 73;
    }

    MessageBoxW(nullptr,
        L"ZERO Reference Experience is connected.\n\n"
        L"SDK READY: PASS\nResume activity: SET\nAchievement: SET\n\n"
        L"Exit this window to return cleanly to ZERO.",
        L"ZERO Reference Experience", MB_OK | MB_ICONINFORMATION);
    return 0;
}
