#include "App.h"
#include "ApplicationServices.h"
#include "FirstBootService.h"
#include "FirstBootWizard.h"
#include "PlatformPaths.h"
#include <windows.h>

namespace {

bool ApplyFirstBootSettings(zero::SettingsStore& settingsStore, const zero::FirstBootState& state) {
    auto settings = settingsStore.Load();
    settings.profileName = state.profileName.empty() ? "Player" : state.profileName;
    settings.volume = static_cast<int>(state.volume > 100 ? 100 : state.volume);
    return settingsStore.Save(settings);
}

bool RunFirstBootIfRequired(HINSTANCE instance,
                            const std::filesystem::path& dataRoot,
                            zero::SettingsStore& settingsStore) {
    zero::FirstBootService firstBoot(dataRoot);
    if (!firstBoot.IsRequired()) return true;

    zero::FirstBootWizard wizard(instance, firstBoot);
    if (!wizard.Run()) return false;

    if (!ApplyFirstBootSettings(settingsStore, firstBoot.Load())) {
        MessageBoxW(nullptr,
            L"ZERO completed setup but could not persist profile and volume settings. Restart ZERO after checking local storage access.",
            L"ZERO Setup", MB_OK | MB_ICONERROR);
        return false;
    }
    return true;
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const auto dataRoot = zero::PlatformPaths::DataRoot();
    zero::ApplicationServices services(dataRoot);
    if (!RunFirstBootIfRequired(instance, dataRoot, services.Settings())) return 0;

    zero::App app(instance, services);
    return app.Run();
}
