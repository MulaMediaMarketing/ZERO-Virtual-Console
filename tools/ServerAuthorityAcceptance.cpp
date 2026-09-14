#include "FriendsProvider.h"
#include "StoreProvider.h"
#include "StoreSettingsFirstBootExperience.h"

#include <iostream>

namespace {

bool Require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}

} // namespace

int main() {
    bool ok = true;

    zero::DisconnectedFriendsProvider friends;
    ok &= Require(friends.State() == zero::FriendsProviderState::Disconnected,
                  "Default Friends provider must fail closed as disconnected.");
    ok &= Require(friends.Friends().empty(),
                  "Disconnected Friends provider must not manufacture friend/presence state.");
    friends.Refresh();
    ok &= Require(friends.State() == zero::FriendsProviderState::Disconnected,
                  "Refresh must not promote disconnected Friends without an authority.");
    ok &= Require(friends.Friends().empty(),
                  "Refresh must not manufacture joinable Friends state.");

    zero::DisconnectedStoreProvider store;
    ok &= Require(store.State() == zero::StoreProviderState::Disconnected,
                  "Default Store provider must fail closed as disconnected.");
    ok &= Require(store.Products().empty(),
                  "Disconnected Store provider must not manufacture catalog or entitlement state.");
    store.Refresh();
    ok &= Require(store.State() == zero::StoreProviderState::Disconnected,
                  "Refresh must not promote disconnected Store without an authority.");
    ok &= Require(store.Products().empty(),
                  "Refresh must not manufacture ownership or purchase state.");

    zero::StoreExperienceState storeUx;
    zero::ClampStoreSelection(storeUx, store.State(), store.Products().size());
    ok &= Require(storeUx.mode == zero::StoreExperienceMode::Disconnected,
                  "Store UX must truthfully expose disconnected authority state.");
    ok &= Require(storeUx.selectedIndex == 0 && storeUx.scrollOffset == 0,
                  "Disconnected Store state must not retain actionable local selection state.");

    if (!ok) return 2;
    std::cout << "ZERO server-authority fail-closed acceptance: PASS\n";
    return 0;
}
