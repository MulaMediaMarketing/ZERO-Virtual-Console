#include "v5/DiscoverDomain.h"
#include <iostream>
#include <vector>

using namespace zero::v5;

namespace {
DiscoverItem item(const char* id, ContentClass contentClass, std::uint64_t popularity) {
    DiscoverItem value;
    value.identity = {id, std::string("pkg.") + id, "publisher.zero", contentClass};
    value.title = id;
    value.region = "US";
    value.ageRating = 13;
    value.popularityScore = popularity;
    value.moderation = ModerationState::Approved;
    value.visibility = VisibilityState::Public;
    return value;
}
}

int main() {
    DiscoverContext context{"acct-1", "US", 17};

    auto freeGame = item("free", ContentClass::FreeToPlay, 100);
    auto community = item("community", ContentClass::Community, 80);
    community.recommendationScore = 900;
    community.friendActivityScore = 4;
    community.publishedEpochSeconds = 500;

    auto premium = item("premium", ContentClass::Premium, 200);
    auto blocked = item("blocked", ContentClass::Community, 1000);
    blocked.moderation = ModerationState::Removed;

    auto mature = item("mature", ContentClass::FreeToPlay, 600);
    mature.ageRating = 18;

    std::vector<DiscoverItem> catalog{freeGame, community, premium, blocked, mature};

    const auto freeFeed = DiscoverDomain::Build(DiscoverSection::FreeToPlay, context, catalog);
    if (freeFeed.items.size() != 2) return 10;
    if (freeFeed.items[0].identity.contentId != "free") return 11;
    if (!DiscoverDomain::RequiresEntitlementGrant(freeFeed.items[0])) return 12;
    if (DiscoverDomain::DefaultEntitlementType(freeFeed.items[0]) != EntitlementType::Free) return 13;
    if (DiscoverDomain::DefaultEntitlementType(premium) != EntitlementType::Purchased) return 14;

    const auto recommended = DiscoverDomain::Build(DiscoverSection::Recommended, context, catalog);
    if (recommended.items.size() != 1 || recommended.items[0].identity.contentId != "community") return 20;

    const auto premiumFeed = DiscoverDomain::Build(DiscoverSection::PremiumReleases, context, catalog);
    if (premiumFeed.items.size() != 1 || premiumFeed.items[0].identity.contentId != "premium") return 21;

    DiscoverContext adult = context;
    adult.age = 21;
    const auto adultFree = DiscoverDomain::Build(DiscoverSection::FreeToPlay, adult, catalog);
    if (adultFree.items.size() != 3 || adultFree.items[0].identity.contentId != "mature") return 22;

    DiscoverContext wrongRegion = context;
    wrongRegion.region = "CA";
    if (!DiscoverDomain::Build(DiscoverSection::Trending, wrongRegion, catalog).items.empty()) return 23;

    std::cout << "ZERO V5 Discover / Free-to-Play / Community acceptance: PASS\n";
    return 0;
}
