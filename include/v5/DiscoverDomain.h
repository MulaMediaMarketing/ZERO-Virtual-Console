#pragma once

#include "ContentModel.h"
#include <cstdint>
#include <string>
#include <vector>

namespace zero::v5 {

enum class ModerationState : std::uint8_t {
    Pending,
    Approved,
    Restricted,
    Removed
};

enum class VisibilityState : std::uint8_t {
    Private,
    Unlisted,
    Public
};

enum class DiscoverSection : std::uint8_t {
    ContinuePlaying,
    Trending,
    FreeToPlay,
    ZeroOriginals,
    PremiumReleases,
    NewCommunityGames,
    PopularWithFriends,
    RecentlyUpdated,
    ShortExperiences,
    Multiplayer,
    TwoDGames,
    Experimental,
    Recommended
};

struct DiscoverItem {
    ContentIdentity identity;
    std::string title;
    std::string creatorId;
    std::string version;
    std::string genre;
    std::string region;
    std::uint32_t ageRating{0};
    std::uint64_t popularityScore{0};
    std::uint64_t friendActivityScore{0};
    std::uint64_t recentPlayScore{0};
    std::uint64_t recommendationScore{0};
    std::uint64_t publishedEpochSeconds{0};
    std::uint64_t updatedEpochSeconds{0};
    bool multiplayer{false};
    bool twoDimensional{false};
    bool experimental{false};
    bool shortExperience{false};
    bool cloudReady{false};
    ModerationState moderation{ModerationState::Pending};
    VisibilityState visibility{VisibilityState::Private};
};

struct DiscoverContext {
    std::string accountId;
    std::string region;
    std::uint32_t age{0};
};

struct DiscoverFeed {
    DiscoverSection section{DiscoverSection::Trending};
    std::vector<DiscoverItem> items;
};

class DiscoverDomain {
public:
    static bool Eligible(const DiscoverItem& item, const DiscoverContext& context) noexcept;
    static bool RequiresEntitlementGrant(const DiscoverItem& item) noexcept;
    static EntitlementType DefaultEntitlementType(const DiscoverItem& item) noexcept;
    static DiscoverFeed Build(DiscoverSection section,
                              const DiscoverContext& context,
                              std::vector<DiscoverItem> items,
                              std::size_t limit = 24);
};

} // namespace zero::v5
