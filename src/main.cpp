#include "App.h"
#include "FirstBootService.h"
#include "FirstBootWizard.h"
#include "Settings.h"
#include <windows.h>
#include <shlobj.h>
#include <filesystem>

namespace {
std::filesystem::path zeroRoot() {
    PWSTR p = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &p))) {
        std::filesystem::path root = std::filesystem::path(p) / "ZERO";
        CoTaskMemFree(p);
        return root;
    }
    return std::filesystem::current_path() / "ZeroData";
}

bool applyFirstBootSettings(const std::filesystem::path& root, const zero::FirstBootState& state) {
    zero::SettingsStore settingsStore(root);
    auto settings = settingsStore.Load();
    settings.profileName = state.profileName.empty() ? "Player" : state.profileName;
    settings.volume = static_cast<int>(state.volume > 100 ? 100 : state.volume);
    return settingsStore.Save(settings);
}
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    const auto root = zeroRoot();
    zero::FirstBootService firstBoot(root);
    if (firstBoot.IsRequired()) {
        zero::FirstBootWizard wizard(hInstance, firstBoot);
        if (!wizard.Run()) return 0;
        if (!applyFirstBootSettings(root, firstBoot.Load())) {
            MessageBoxW(nullptr,
                L"ZERO completed setup but could not persist the profile and volume settings. Setup will remain saved; restart ZERO after checking local storage access.",
                L"ZERO Setup", MB_OK | MB_ICONERROR);
            return 1;
        }
    }

    zero::App app(hInstance);
    return app.Run();
}
