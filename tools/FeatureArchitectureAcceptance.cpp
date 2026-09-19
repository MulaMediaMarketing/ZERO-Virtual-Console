#include "ApplicationServices.h"
#include "features/achievements/AchievementsFeature.h"
#include "features/capture/CaptureFeature.h"
#include "features/devices/DevicesFeature.h"
#include "features/downloads/DownloadsFeature.h"
#include "features/friends/FriendsFeature.h"
#include "features/home/HomeFeature.h"
#include "features/library/LibraryFeature.h"
#include "features/notifications/NotificationsFeature.h"
#include "features/profile/ProfileFeature.h"
#include "features/settings/SettingsFeature.h"
#include "features/store/StoreFeature.h"
#include "shell/ShellModule.h"
#include "ui/FeatureScene.h"
#include <iostream>
#include <type_traits>

namespace {
int failures = 0;

void Check(bool condition, const char* message) {
    if (condition) return;
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
}
}

int main() {
    using namespace zero;

    static_assert(!std::is_copy_constructible_v<ApplicationServices>,
                  "ApplicationServices must remain a single non-copyable composition root");
    static_assert(std::is_default_constructible_v<features::home::HomeViewModel>);
    static_assert(std::is_default_constructible_v<features::library::LibraryViewModel>);
    static_assert(std::is_default_constructible_v<features::store::StoreViewModel>);
    static_assert(std::is_default_constructible_v<features::downloads::DownloadsViewModel>);
    static_assert(std::is_default_constructible_v<features::friends::FriendsViewModel>);
    static_assert(std::is_default_constructible_v<features::achievements::AchievementsViewModel>);
    static_assert(std::is_default_constructible_v<features::capture::CaptureViewModel>);
    static_assert(std::is_default_constructible_v<features::profile::ProfileViewModel>);
    static_assert(std::is_default_constructible_v<features::devices::DevicesViewModel>);
    static_assert(std::is_default_constructible_v<features::notifications::NotificationsViewModel>);
    static_assert(std::is_default_constructible_v<features::settings::SettingsViewModel>);

    features::home::HomeViewModel home;
    Check(home.empty, "Home must default to truthful empty state");

    features::library::LibraryViewModel library;
    Check(library.empty, "Library must default to truthful empty state");

    features::store::StoreViewModel store;
    Check(store.providerState == StoreProviderState::Disconnected,
          "Store must default disconnected rather than fabricate catalog state");
    Check(store.mode == StoreExperienceMode::Disconnected,
          "Store experience must default disconnected");

    features::friends::FriendsViewModel friends;
    Check(friends.providerState == FriendsProviderState::Disconnected,
          "Friends must default disconnected rather than fabricate presence");

    features::devices::DevicesViewModel devices;
    Check(devices.localPcAvailable, "Local PC must remain available");
    Check(!devices.remoteDeviceServiceConnected,
          "Remote device service must default disconnected");

    features::settings::SettingsViewModel settings;
    Check(!settings.shareActivity && !settings.shareAchievements && !settings.sharePlaytime,
          "Privacy projection must default private");

    features::notifications::NotificationsViewModel notifications;
    Check(features::notifications::NotificationsView::IsEmpty(notifications),
          "Notifications must not synthesize alerts");

    ui::FeatureScene scene;
    scene.AddPanel({0, 0, 100, 100}, 16.0f, ui::SurfaceRole::Card, true);
    scene.AddText(L"ZERO", {10, 10, 90, 40}, true, ui::TextRole::Primary);
    Check(scene.panels.size() == 1 && scene.text.size() == 1,
          "Feature scene must accept declarative panel/text projection");
    Check(scene.panels.front().focused,
          "Feature scene must preserve focus state for controller/keyboard rendering");

    shell::ShellIntent none;
    Check(none.type == shell::ShellIntentType::None,
          "Shell intent must default to no action");

    if (failures != 0) {
        std::cerr << "ZERO feature architecture acceptance: FAIL (" << failures << ")\n";
        return 2;
    }

    std::cout << "ZERO feature architecture acceptance: PASS\n";
    return 0;
}
