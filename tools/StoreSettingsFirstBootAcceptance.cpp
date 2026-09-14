#include "StoreSettingsFirstBootExperience.h"
#include <iostream>
#include <vector>

using namespace zero;

namespace {
bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}
}

int main() {
    bool ok = true;

    ok &= expect(StoreModeFor(StoreProviderState::Disconnected, 0) == StoreExperienceMode::Disconnected,
                 "disconnected store must remain disconnected");
    ok &= expect(StoreModeFor(StoreProviderState::Error, 3) == StoreExperienceMode::Error,
                 "provider error must override product count");
    ok &= expect(StoreModeFor(StoreProviderState::Ready, 0) == StoreExperienceMode::Empty,
                 "ready empty provider must produce truthful empty state");
    ok &= expect(StoreModeFor(StoreProviderState::Ready, 2) == StoreExperienceMode::Browsing,
                 "ready provider with products must browse");

    StoreProduct purchasable{"sku-1", "pkg.game", "Game", "1.0", "desc", "$9.99", EntitlementState::NotOwned};
    StoreProduct owned{"sku-2", "pkg.owned", "Owned", "1.0", "desc", "$4.99", EntitlementState::Owned};
    StoreProduct invalid{"", "", "Invalid", "1.0", "desc", "$1.00", EntitlementState::NotOwned};
    ok &= expect(StoreProductCanLaunchPurchase(purchasable), "valid not-owned product must be purchasable");
    ok &= expect(!StoreProductCanLaunchPurchase(owned), "owned product must not launch purchase");
    ok &= expect(StoreProductIsOwned(owned), "owned entitlement must be recognized");
    ok &= expect(!StoreProductCanLaunchPurchase(invalid), "missing IDs must fail closed");

    StoreExperienceState store{};
    store.selectedIndex = 9;
    ClampStoreSelection(store, StoreProviderState::Ready, 2);
    ok &= expect(store.selectedIndex == 1, "store selection must clamp to last product");
    ClampStoreSelection(store, StoreProviderState::Disconnected, 2);
    ok &= expect(store.selectedIndex == 0 && store.mode == StoreExperienceMode::Disconnected,
                 "disconnected store must clear stale selection");

    SettingsExperienceState settings{};
    settings.selectedRow = 99;
    ClampSettingsSelection(settings);
    ok &= expect(settings.selectedRow == SettingsRowCount() - 1, "settings selection must clamp");
    ok &= expect(!MoveSettingsDown(settings), "settings must clamp at bottom");
    settings.selectedRow = 0;
    ok &= expect(!MoveSettingsUp(settings), "settings must clamp at top");
    ok &= expect(AdjustVolume(98, 5) == 100, "volume must clamp high");
    ok &= expect(AdjustVolume(2, -5) == 0, "volume must clamp low");

    FirstBootState fresh{};
    ok &= expect(FirstBootModeFor(fresh) == FirstBootExperienceMode::Required,
                 "fresh install must require first boot");
    ok &= expect(!FirstBootCanComplete(fresh), "incomplete first boot cannot complete");

    FirstBootState partial = fresh;
    partial.profileName = "PlayerOne";
    partial.controllerConfirmed = true;
    ok &= expect(FirstBootModeFor(partial) == FirstBootExperienceMode::InProgress,
                 "partial setup must remain in progress");

    FirstBootState ready = partial;
    ready.displayConfirmed = true;
    ready.audioConfirmed = true;
    ready.volume = 80;
    ok &= expect(FirstBootRequirementsSatisfied(ready), "required confirmations must satisfy first boot");
    ok &= expect(FirstBootCanComplete(ready), "satisfied incomplete state must be completable");

    FirstBootState completed = ready;
    completed.completed = true;
    ok &= expect(FirstBootModeFor(completed) == FirstBootExperienceMode::Complete,
                 "valid completed state must remain complete");

    FirstBootState corrupt = completed;
    corrupt.audioConfirmed = false;
    corrupt.volume = 250;
    corrupt = NormalizeFirstBootState(corrupt);
    ok &= expect(corrupt.volume == 100, "first boot volume must normalize to 100 max");
    ok &= expect(!corrupt.completed, "invalid completed state must fail closed and require recovery");

    if (!ok) return 1;
    std::cout << "PASS: ZERO Store Settings and First Boot production UX contract\n";
    return 0;
}
