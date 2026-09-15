#include "v5/DiscoverExperience.h"
#include <algorithm>
#include <utility>

namespace zero::v5 {

bool DisconnectedDiscoverFeedProvider::Fetch(DiscoverSection,
                                             const DiscoverContext&,
                                             std::vector<DiscoverItem>&,
                                             std::string& error) {
    error = "Discover service is not connected.";
    return false;
}

DiscoverExperienceController::DiscoverExperienceController(IDiscoverFeedProvider& provider,
                                                           DiscoverContext context)
    : provider_(provider), context_(std::move(context)) {
    snapshot_.message = "Discover has not been refreshed yet.";
}

PageSnapshot DiscoverExperienceController::Snapshot() const {
    PageSnapshot page;
    page.page = ShellPage::Discover;
    page.revision = snapshot_.revision;
    page.loading = false;
    page.available = snapshot_.status == DiscoverExperienceStatus::Ready;
    page.status = snapshot_.message;
    return page;
}

bool DiscoverExperienceController::Refresh(std::string& error) {
    std::vector<DiscoverItem> items;
    if (!provider_.Fetch(snapshot_.section, context_, items, error)) {
        snapshot_.status = DiscoverExperienceStatus::Disconnected;
        snapshot_.items.clear();
        snapshot_.selectedIndex = 0;
        snapshot_.message = error.empty() ? "Discover service is unavailable." : error;
        ++snapshot_.revision;
        return false;
    }

    const auto feed = DiscoverDomain::Build(snapshot_.section, context_, std::move(items));
    snapshot_.items = feed.items;
    snapshot_.selectedIndex = snapshot_.items.empty()
        ? 0
        : std::min(snapshot_.selectedIndex, snapshot_.items.size() - 1);
    snapshot_.status = DiscoverExperienceStatus::Ready;
    snapshot_.message = snapshot_.items.empty() ? "No eligible content is available in this section." : std::string{};
    ++snapshot_.revision;
    error.clear();
    return true;
}

bool DiscoverExperienceController::SetSection(DiscoverSection section, std::string& error) {
    snapshot_.section = section;
    snapshot_.selectedIndex = 0;
    return Refresh(error);
}

bool DiscoverExperienceController::MoveSelection(int delta) noexcept {
    if (snapshot_.items.empty() || delta == 0) return false;
    const auto current = static_cast<long long>(snapshot_.selectedIndex);
    const auto last = static_cast<long long>(snapshot_.items.size() - 1);
    const auto next = std::clamp(current + static_cast<long long>(delta), 0LL, last);
    if (next == current) return false;
    snapshot_.selectedIndex = static_cast<std::size_t>(next);
    ++snapshot_.revision;
    return true;
}

bool DiscoverExperienceController::Execute(const ShellCommand& command, std::string& error) {
    if (command.target != ShellPage::Discover) {
        error = "Discover command was routed to the wrong controller.";
        return false;
    }

    switch (command.type) {
        case ShellCommandType::Activate:
        case ShellCommandType::Refresh:
            return Refresh(error);
        case ShellCommandType::Select:
            if (snapshot_.status != DiscoverExperienceStatus::Ready || snapshot_.items.empty()) {
                error = "Discover selection is unavailable.";
                return false;
            }
            error.clear();
            return true;
        case ShellCommandType::PrimaryAction:
        case ShellCommandType::SecondaryAction:
        case ShellCommandType::Search:
            if (snapshot_.status != DiscoverExperienceStatus::Ready) {
                error = snapshot_.message.empty() ? "Discover is unavailable." : snapshot_.message;
                return false;
            }
            error.clear();
            return true;
        default:
            error = "Unsupported Discover command.";
            return false;
    }
}

} // namespace zero::v5
