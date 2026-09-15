#include "v5/DiscoverExperience.h"
#include <iostream>
#include <string>
#include <vector>

using namespace zero::v5;

namespace {
class StubProvider final : public IDiscoverFeedProvider {
public:
    bool connected{true};

    bool Fetch(DiscoverSection,
               const DiscoverContext&,
               std::vector<DiscoverItem>& items,
               std::string& error) override {
        if (!connected) {
            error = "stub disconnected";
            return false;
        }

        DiscoverItem freeGame;
        freeGame.identity = {"content.free", "pkg.free", "publisher.zero", ContentClass::FreeToPlay};
        freeGame.title = "Free Game";
        freeGame.version = "1.0.0";
        freeGame.region = "US";
        freeGame.ageRating = 12;
        freeGame.popularityScore = 100;
        freeGame.moderation = ModerationState::Approved;
        freeGame.visibility = VisibilityState::Public;

        DiscoverItem removed = freeGame;
        removed.identity = {"content.removed", "pkg.removed", "publisher.zero", ContentClass::FreeToPlay};
        removed.title = "Removed Game";
        removed.moderation = ModerationState::Removed;

        items = {freeGame, removed};
        error.clear();
        return true;
    }
};
}

int main() {
    DiscoverContext context;
    context.accountId = "acct-1";
    context.region = "US";
    context.age = 18;

    DisconnectedDiscoverFeedProvider disconnected;
    DiscoverExperienceController blocked(disconnected, context);
    std::string error;
    if (blocked.Refresh(error)) return 10;
    if (blocked.Snapshot().available) return 11;
    if (blocked.Experience().status != DiscoverExperienceStatus::Disconnected) return 12;

    StubProvider provider;
    DiscoverExperienceController discover(provider, context);
    if (!discover.Refresh(error)) return 20;
    if (!discover.Snapshot().available) return 21;
    if (discover.Experience().items.size() != 1) return 22;
    if (discover.Experience().items.front().identity.contentId != "content.free") return 23;
    if (!DiscoverDomain::RequiresEntitlementGrant(discover.Experience().items.front())) return 24;
    if (DiscoverDomain::DefaultEntitlementType(discover.Experience().items.front()) != EntitlementType::Free) return 25;

    ShellCommand select;
    select.type = ShellCommandType::Select;
    select.target = ShellPage::Discover;
    if (!discover.Execute(select, error)) return 26;

    const auto revision = discover.Experience().revision;
    if (discover.MoveSelection(1)) return 27;
    if (discover.Experience().revision != revision) return 28;

    provider.connected = false;
    ShellCommand refresh;
    refresh.type = ShellCommandType::Refresh;
    refresh.target = ShellPage::Discover;
    if (discover.Execute(refresh, error)) return 30;
    if (discover.Snapshot().available) return 31;
    if (!discover.Experience().items.empty()) return 32;

    std::cout << "ZERO V5 Discover production experience acceptance: PASS\n";
    return 0;
}
