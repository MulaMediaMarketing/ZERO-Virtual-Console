#include "App.h"
#include "PlatformPaths.h"
#include <Xinput.h>
#include <shellapi.h>
#include <algorithm>
#include <filesystem>

using Microsoft::WRL::ComPtr;

namespace zero {
namespace {
std::wstring BytesLabel(uint64_t bytes) {
    constexpr uint64_t GiB = 1024ull * 1024ull * 1024ull;
    constexpr uint64_t MiB = 1024ull * 1024ull;
    if (bytes >= GiB) return std::to_wstring(bytes / GiB) + L" GB";
    if (bytes >= MiB) return std::to_wstring(bytes / MiB) + L" MB";
    return std::to_wstring(bytes / 1024ull) + L" KB";
}

uint64_t DirectorySize(const std::filesystem::path& root) {
    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) return 0;
    uint64_t total = 0;
    for (std::filesystem::recursive_directory_iterator it(
             root, std::filesystem::directory_options::skip_permission_denied, ec), end;
         !ec && it != end; it.increment(ec)) {
        if (it->is_regular_file(ec)) {
            const auto size = it->file_size(ec);
            if (!ec) total += size;
        }
        ec.clear();
    }
    return total;
}

size_t CountJsonReports(const std::filesystem::path& root) {
    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) return 0;
    size_t count = 0;
    for (std::filesystem::recursive_directory_iterator it(
             root, std::filesystem::directory_options::skip_permission_denied, ec), end;
         !ec && it != end; it.increment(ec)) {
        if (it->is_regular_file(ec) && _wcsicmp(it->path().extension().c_str(), L".json") == 0) ++count;
        ec.clear();
    }
    return count;
}

std::wstring PrivacyLabel(const UserSettings& settings) {
    const int shared = static_cast<int>(settings.shareActivity) +
                       static_cast<int>(settings.shareAchievements) +
                       static_cast<int>(settings.sharePlaytime);
    if (shared == 0) return L"Private";
    if (shared == 3) return L"Share activity, achievements and playtime";
    std::wstring label = L"Custom: ";
    bool first = true;
    if (settings.shareActivity) { label += L"activity"; first = false; }
    if (settings.shareAchievements) { if (!first) label += L", "; label += L"achievements"; first = false; }
    if (settings.sharePlaytime) { if (!first) label += L", "; label += L"playtime"; }
    return label;
}
}

void App::RefreshSettingsTelemetry() {
    XINPUT_STATE state{};
    settingsControllerConnected_ = XInputGetState(0, &state) == ERROR_SUCCESS;

    RECT rc{};
    GetClientRect(hwnd_, &rc);
    settingsDisplayWidth_ = static_cast<unsigned>(std::max<LONG>(0, rc.right - rc.left));
    settingsDisplayHeight_ = static_cast<unsigned>(std::max<LONG>(0, rc.bottom - rc.top));

    const auto root = PlatformPaths::DataRoot();
    settingsStorageBytes_ = DirectorySize(root);
    settingsCrashReportCount_ = CountJsonReports(PlatformPaths::CrashReportsRoot());

    std::error_code ec;
    const auto space = std::filesystem::space(root, ec);
    settingsFreeBytes_ = ec ? 0 : space.available;
}

void App::OpenSettingsLocation(bool diagnosticsOnly) {
    const auto target = diagnosticsOnly ? PlatformPaths::CrashReportsRoot() : PlatformPaths::DataRoot();
    std::error_code ec;
    std::filesystem::create_directories(target, ec);
    if (ec) {
        status_ = L"ZERO could not open that system location.";
        return;
    }
    const auto result = reinterpret_cast<INT_PTR>(
        ShellExecuteW(hwnd_, L"open", target.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
    if (result <= 32) status_ = L"Windows could not open that system location.";
}

void App::HandleSettingsInput(const InputSnapshot& in) {
    if (in.up && MoveSettingsUp(settingsUx_)) NotifyFocusMoved();
    if (in.down && MoveSettingsDown(settingsUx_)) NotifyFocusMoved();

    const auto row = static_cast<SettingsExperienceRow>(settingsUx_.selectedRow);
    if (row == SettingsExperienceRow::Volume && (in.left || in.right)) {
        settings_.volume = AdjustVolume(settings_.volume, in.left ? -5 : 5);
        if (!settingsStore_.Save(settings_)) {
            status_ = L"ZERO could not persist its volume preference.";
        } else {
            status_ = L"ZERO volume preference: " + std::to_wstring(settings_.volume) + L"%.";
        }
        return;
    }

    if (!in.select && !in.action) return;

    switch (row) {
        case SettingsExperienceRow::Profile:
            status_ = L"Local ZERO identity is active. Online identity editing appears only when the authoritative ZERO identity service is connected.";
            break;
        case SettingsExperienceRow::Privacy: {
            const bool allShared = settings_.shareActivity && settings_.shareAchievements && settings_.sharePlaytime;
            settings_.shareActivity = !allShared;
            settings_.shareAchievements = !allShared;
            settings_.sharePlaytime = !allShared;
            if (!settingsStore_.Save(settings_)) {
                status_ = L"ZERO could not persist privacy controls.";
            } else {
                status_ = allShared ? L"Profile activity sharing disabled." : L"Profile activity sharing enabled for connected ZERO services.";
            }
            break;
        }
        case SettingsExperienceRow::Volume:
            settings_.volume = AdjustVolume(settings_.volume, 5);
            if (!settingsStore_.Save(settings_)) {
                status_ = L"ZERO could not persist its volume preference.";
            } else {
                status_ = L"ZERO volume preference: " + std::to_wstring(settings_.volume) + L"%.";
            }
            break;
        case SettingsExperienceRow::ReducedMotion:
            settings_.reducedMotion = !settings_.reducedMotion;
            shellUx_.SetReducedMotion(settings_.reducedMotion);
            if (!settingsStore_.Save(settings_)) {
                status_ = L"ZERO could not persist motion settings.";
            } else {
                status_ = settings_.reducedMotion ? L"Reduced Motion enabled." : L"Reduced Motion disabled.";
            }
            break;
        case SettingsExperienceRow::Controller:
            RefreshSettingsTelemetry();
            status_ = settingsControllerConnected_ ? L"Controller 1 is connected." : L"No XInput controller is currently connected.";
            break;
        case SettingsExperienceRow::Display:
            EnterBorderlessFullscreen();
            RefreshSettingsTelemetry();
            status_ = L"Display state refreshed.";
            break;
        case SettingsExperienceRow::Storage:
            OpenSettingsLocation(false);
            break;
        case SettingsExperienceRow::Diagnostics:
            OpenSettingsLocation(true);
            break;
        case SettingsExperienceRow::About:
            status_ = L"ZERO Player · ZERO Core V5 · Windows 11 x64 · authoritative V5 runtime and shell.";
            break;
        case SettingsExperienceRow::Count:
            break;
    }
    NotifyFocusMoved();
}

void App::DrawSettings(float width, float height) {
    ClampSettingsSelection(settingsUx_);
    const auto profile = identity_.CurrentProfile();

    DrawTextLine(L"Settings", 62, 132, 500, 60, true);
    DrawTextLine(L"System controls and truthful local console status", 64, 190, 720, 30, false, brushMuted_.Get());

    struct Row {
        const wchar_t* label;
        std::wstring value;
        const wchar_t* hint;
    };

    const std::wstring display = std::to_wstring(settingsDisplayWidth_) + L" × " +
                                 std::to_wstring(settingsDisplayHeight_) + L" borderless";
    const std::wstring storage = BytesLabel(settingsStorageBytes_) + L" used · " +
                                 BytesLabel(settingsFreeBytes_) + L" free";
    const std::wstring diagnostics = std::to_wstring(settingsCrashReportCount_) + L" crash report" +
                                     (settingsCrashReportCount_ == 1 ? L"" : L"s");
    const std::wstring identityLabel = profile.zeroId.empty() ? L"Local identity unavailable" : Widen(profile.zeroId);

    Row rows[] = {
        {L"Profile", Widen(profile.displayName.empty() ? settings_.profileName : profile.displayName) + L" · " + identityLabel, L"A Info"},
        {L"Privacy", PrivacyLabel(settings_), L"A Toggle sharing"},
        {L"ZERO Volume", std::to_wstring(settings_.volume) + L"% preference", L"Left / Right Adjust"},
        {L"Reduced Motion", settings_.reducedMotion ? L"On" : L"Off", L"A Toggle"},
        {L"Controller", settingsControllerConnected_ ? L"Controller 1 connected" : L"No XInput controller", L"A Refresh"},
        {L"Display", display, L"A Reapply fullscreen"},
        {L"Storage", storage, L"A Open ZERO data"},
        {L"Diagnostics", diagnostics, L"A Open crash reports"},
        {L"System / About", L"ZERO Core V5 · Windows 11 x64", L"A Details"}
    };

    const float startY = 238.0f;
    const float rowHeight = 55.0f;
    for (size_t i = 0; i < SettingsRowCount(); ++i) {
        const float y = startY + static_cast<float>(i) * (rowHeight + 6.0f);
        const auto rect = D2D1::RectF(62, y, width - 62, y + rowHeight);
        DrawRoundedCard(rect, 17, brushCard_.Get());
        if (i == settingsUx_.selectedRow) DrawFocusRing(rect, 17);
        DrawTextLine(rows[i].label, 88, y + 8, 210, 26, false);
        DrawTextLine(rows[i].value, 300, y + 8, std::max(240.0f, width - 700.0f), 30, false, brushMuted_.Get());
        DrawTextLine(rows[i].hint, width - 300, y + 8, 210, 28, false, brushMuted_.Get());
    }

    DrawTextLine(L"Privacy defaults to Private. Up / Down Navigate · A Select · LB / RB Switch destination · B Home",
                 64, height - 68, width - 128, 28, false, brushMuted_.Get());
}

} // namespace zero
