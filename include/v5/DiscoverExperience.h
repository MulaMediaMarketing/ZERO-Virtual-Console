#pragma once

#include "v5/DiscoverDomain.h"
#include "v5/ShellKernel.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace zero::v5 {

enum class DiscoverExperienceStatus : std::uint8_t {
    Disconnected,
    Ready,
    Error
};

struct DiscoverExperienceSnapshot {
    DiscoverExperienceStatus status{DiscoverExperienceStatus::Disconnected};
    DiscoverSection section{DiscoverSection::Trending};
    std::vector<DiscoverItem> items;
    std::size_t selectedIndex{0};
    std::uint64_t revision{0};
    std::string message;
};

class IDiscoverFeedProvider {
public:
    virtual ~IDiscoverFeedProvider() = default;
    virtual bool Fetch(DiscoverSection section,
                       const DiscoverContext& context,
                       std::vector<DiscoverItem>& items,
                       std::string& error) = 0;
};

class DisconnectedDiscoverFeedProvider final : public IDiscoverFeedProvider {
public:
    bool Fetch(DiscoverSection,
               const DiscoverContext&,
               std::vector<DiscoverItem>&,
               std::string& error) override;
};

class DiscoverExperienceController final : public IShellPageController {
public:
    DiscoverExperienceController(IDiscoverFeedProvider& provider, DiscoverContext context);

    ShellPage Page() const noexcept override { return ShellPage::Discover; }
    PageSnapshot Snapshot() const override;
    bool Execute(const ShellCommand& command, std::string& error) override;

    const DiscoverExperienceSnapshot& Experience() const noexcept { return snapshot_; }
    bool Refresh(std::string& error);
    bool SetSection(DiscoverSection section, std::string& error);
    bool MoveSelection(int delta) noexcept;

private:
    IDiscoverFeedProvider& provider_;
    DiscoverContext context_;
    DiscoverExperienceSnapshot snapshot_;
};

} // namespace zero::v5
