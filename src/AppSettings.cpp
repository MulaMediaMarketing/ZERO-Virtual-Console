#include "App.h"
#include <Xinput.h>
#include <shlobj.h>
#include <shellapi.h>
#include <algorithm>
#include <filesystem>

using Microsoft::WRL::ComPtr;

namespace zero {
namespace {
std::filesystem::path LocalRoot() {
    PWSTR p = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &p))) {
        std::filesystem::path out = std::filesystem::path(p) / "ZERO";
        CoTaskMemFree(p);
        return out;
    }
    return std::filesystem::current_path() / "ZeroData";
}

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
    for (std::filesystem::recursive_directory_iterator it(root, std::filesystem::directory_options::skip_permission_denied, ec), end;
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
    for (std::filesystem::recursive_directory_iterator it(root, std::filesystem::directory_options::skip_permission_denied, ec), end;
         !ec && it != end; it.increment(ec)) {
        if (it->is_regular_file(ec) && _wcsicmp(it->path().extension().c_str(), L".json") == 0) ++count;
        ec.clear();
    }
    return count;
}
}

void App::RefreshSettingsTelemetry() {
    XINPUT_STATE state{};
    settingsControllerConnected_ = XInputGetState(0, &state) == ERROR_SUCCESS;

    RECT rc{};
    GetClientRect(hwnd_, &rc);
    settingsDisplayWidth_ = static_cast<unsigned>(std::max<LONG>(0, rc.right - rc.left));
    settingsDisplayHeight_ = static_cast<unsigned>(std::max<LONG>(0, rc.bottom - rc.top));

    const auto root = LocalRoot();
    settingsStorageBytes_ = DirectorySize(root);
    settingsCrashReportCount_ = CountJsonReports(root / "CrashReports");

    std::error_code ec;
    const auto space = std::filesystem::space(root, ec);
    settingsFreeBytes_ = ec ? 0 : space.available;
}

void App::OpenSettingsLocation(bool diagnosticsOnly) {
    const auto target = diagnosticsOnly ? (LocalRoot() / "CrashReports") : LocalRoot();
    std::error_code ec;
    std::filesystem::create_directories(target, ec);
    if (ec) {
        status_ = L"ZERO could not open that system location.";
        return;
    }
    const auto result = reinterpret_cast<INT_PTR>(ShellExecuteW(hwnd_, L"open", target.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
    if (result <= 32) status_ = L"Windows could not open that system location.";
}

void App::HandleSettingsInput(const InputSnapshot& in) {
    constexpr size_t kCount = 8;
    if (in.up && selectedSetting_ > 0) {
        --selectedSetting_;
        NotifyFocusMoved();
    }
    if (in.down && selectedSetting_ + 1 < kCount) {
        ++selectedSetting_;
        NotifyFocusMoved();
    }

    if (selectedSetting_ == 1 && (in.left || in.right)) {
        const int delta = in.left ? -5 : 5;
        settings_.volume = std::clamp(settings_.volume + delta, 0, 100);
        if (!settingsStore_.Save(settings_)) status_ = L"ZERO could not persist volume settings.";
        else status_ = L"System volume preference: " + std::to_wstring(settings_.volume) + L"%.";
        return;
    }

    if (!in.select && !in.action) return;

    switch (selectedSetting_) {
        case 0: {
            status_ = L"Profile name editing is available during First Boot. Full profile editing is a later identity slice.";
            break;
        }
        case 1: {
            settings_.volume = std::clamp(settings_.volume + 5, 0, 100);
            if (!settingsStore_.Save(settings_)) status_ = L"ZERO could not persist volume settings.";
            else status_ = L"System volume preference: " + std::to_wstring(settings_.volume) + L"%.";
            break;
        }
        case 2: {
            settings_.reducedMotion = !settings_.reducedMotion;
            shellUx_.SetReducedMotion(settings_.reducedMotion);
            if (!settingsStore_.Save(settings_)) status_ = L"ZERO could not persist motion settings.";
            else status_ = settings_.reducedMotion ? L"Reduced Motion enabled." : L"Reduced Motion disabled.";
            break;
        }
        case 3: {
            RefreshSettingsTelemetry();
            status_ = settingsControllerConnected_ ? L"Controller 1 is connected." : L"No XInput controller is currently connected.";
            break;
        }
        case 4: {
            EnterBorderlessFullscreen();
            RefreshSettingsTelemetry();
            status_ = L"Display state refreshed.";
            break;
        }
        case 5: {
            OpenSettingsLocation(false);
            break;
        }
        case 6: {
            OpenSettingsLocation(true);
            break;
        }
        case 7: {
            status_ = L"ZERO Virtual Console · Runtime V4.1 · Windows 11 x64 · local console shell.";
            break;
        }
    }
    NotifyFocusMoved();
}

void App::DrawSettings(float width, float height) {
    RefreshSettingsTelemetry();
    const auto profile = identity_.CurrentProfile();

    DrawTextLine(L"Settings", 62, 154, 500, 60, true);
    DrawTextLine(L"System controls and local console status", 64, 204, 620, 30, false, brushMuted_.Get());

    struct Row { const wchar_t* label; std::wstring value; const wchar_t* hint; };
    const std::wstring display = std::to_wstring(settingsDisplayWidth_) + L" × " + std::to_wstring(settingsDisplayHeight_) + L" borderless";
    const std::wstring storage = BytesLabel(settingsStorageBytes_) + L" used · " + BytesLabel(settingsFreeBytes_) + L" free";
    const std::wstring diagnostics = std::to_wstring(settingsCrashReportCount_) + L" crash report" + (settingsCrashReportCount_ == 1 ? L"" : L"s");
    const std::wstring identityLabel = profile.zeroId.empty() ? L"Local identity unavailable" : Widen(profile.zeroId);
    Row rows[] = {
        {L"Profile", Widen(profile.displayName.empty() ? settings_.profileName : profile.displayName) + L" · " + identityLabel, L"A Info"},
        {L"Volume", std::to_wstring(settings_.volume) + L"%", L"Left / Right Adjust"},
        {L"Reduced Motion", settings_.reducedMotion ? L"On" : L"Off", L"A Toggle"},
        {L"Controller", settingsControllerConnected_ ? L"Controller 1 connected" : L"No XInput controller", L"A Refresh"},
        {L"Display", display, L"A Reapply fullscreen"},
        {L"Storage", storage, L"A Open ZERO data"},
        {L"Diagnostics", diagnostics, L"A Open crash reports"},
        {L"System / About", L"Runtime V4.1 · Windows 11 x64", L"A Details"}
    };

    const float startY = 258.0f;
    const float rowHeight = 60.0f;
    for (size_t i = 0; i < 8; ++i) {
        const float y = startY + static_cast<float>(i) * (rowHeight + 8.0f);
        const auto rect = D2D1::RectF(62, y, width - 62, y + rowHeight);
        DrawRoundedCard(rect, 18, brushCard_.Get());
        if (i == selectedSetting_) DrawFocusRing(rect, 18);
        DrawTextLine(rows[i].label, 88, y + 10, 210, 26, false);
        DrawTextLine(rows[i].value, 300, y + 10, std::max(240.0f, width - 700.0f), 32, false, brushMuted_.Get());
        DrawTextLine(rows[i].hint, width - 300, y + 10, 210, 28, false, brushMuted_.Get());
    }

    DrawTextLine(L"Up / Down Navigate   ·   A Select   ·   LB / RB Switch destination   ·   B Home", 64, height - 82, width - 128, 30, false, brushMuted_.Get());
}

} // namespace zero
