#include "v5/DiscoverDomain.h"
#include <algorithm>

namespace zero::v5 {

bool DiscoverDomain::Eligible(const DiscoverItem& item,
                              const DiscoverContext& context) noexcept {
    if (item.identity.contentId.empty() || item.identity.packageId.empty() || item.title.empty()) return false;
    if (item.moderation != ModerationState::Approved) return false;
    if (item.visibility != VisibilityState::Public) return false;
    if (!item.region.empty() && !context.region.empty() && item.region != context.region) return false;
    if (context.age < item.ageRating) return false;
    return true;
}

bool DiscoverDomain::RequiresEntitlementGrant(const DiscoverItem&) noexcept {
    // All managed ZERO content requires an entitlement decision. Free-to-play
    // is an entitlement policy, never a price==0 bypass.
    return true;
}

EntitlementType DiscoverDomain::DefaultEntitlementType(const DiscoverItem& item) noexcept {
    switch (item.identity.contentClass) {
        case ContentClass::FreeToPlay:
        case ContentClass::Community:
        case ContentClass::Demo:
        case ContentClass::Experience:
            return EntitlementType::Free;
        case ContentClass::ZeroOriginal:
        case ContentClass::Premium:
        case ContentClass::Dlc:
        case ContentClass::Expansion:
            return EntitlementType::Purchased;
    }
    return EntitlementType::Purchased;
}

DiscoverFeed DiscoverDomain::Build(DiscoverSection section,
                                   const DiscoverContext& context,
                                   std::vector<DiscoverItem> items,
                                   std::size_t limit) {
    items.erase(std::remove_if(items.begin(), items.end(), [&](const DiscoverItem& item) {
        if (!Eligible(item, context)) return true;
        switch (section) {
            case DiscoverSection::ContinuePlaying:
                return item.recentPlayScore == 0;
            case DiscoverSection::Trending:
                return item.popularityScore == 0;
            case DiscoverSection::FreeToPlay:
                return item.identity.contentClass != ContentClass::FreeToPlay &&
                       item.identity.contentClass != ContentClass::Community &&
                       item.identity.contentClass != ContentClass::Experience;
            case DiscoverSection::ZeroOriginals:
                return item.identity.contentClass != ContentClass::ZeroOriginal;
            case DiscoverSection::PremiumReleases:
                return item.identity.contentClass != ContentClass::Premium;
            case DiscoverSection::NewCommunityGames:
                return item.identity.contentClass != ContentClass::Community;
            case DiscoverSection::PopularWithFriends:
                return item.friendActivityScore == 0;
            case DiscoverSection::RecentlyUpdated:
                return item.updatedEpochSeconds == 0;
            case DiscoverSection::ShortExperiences:
                return !item.shortExperience;
            case DiscoverSection::Multiplayer:
                return !item.multiplayer;
            case DiscoverSection::TwoDGames:
                return !item.twoDimensional;
            case DiscoverSection::Experimental:
                return !item.experimental;
            case DiscoverSection::Recommended:
                return item.recommendationScore == 0;
        }
        return false;
    }), items.end());

    const auto score = [section](const DiscoverItem& item) -> std::uint64_t {
        switch (section) {
            case DiscoverSection::ContinuePlaying: return item.recentPlayScore;
            case DiscoverSection::PopularWithFriends: return item.friendActivityScore;
            case DiscoverSection::RecentlyUpdated: return item.updatedEpochSeconds;
            case DiscoverSection::NewCommunityGames: return item.publishedEpochSeconds;
            case DiscoverSection::Recommended: return item.recommendationScore;
            default: return item.popularityScore;
        }
    };

    std::stable_sort(items.begin(), items.end(), [&](const DiscoverItem& a, const DiscoverItem& b) {
        const auto left = score(a);
        const auto right = score(b);
        if (left != right) return left > right;
        return a.identity.contentId < b.identity.contentId;
    });

    if (items.size() > limit) items.resize(limit);
    return DiscoverFeed{section, std::move(items)};
}

} // namespace zero::v5
