#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>

namespace zero {

enum class HomeExperienceMode {
    Empty,
    FeaturedGame
};

enum class HomeSection {
    Featured,
    ContinuePlaying,
    RecentlyPlayed,
    InstalledGames,
    Downloads,
    Captures
};

enum class HomeLayoutDensity {
    Compact,
    Comfortable
};

struct HomeDashboardSummary {
    std::size_t installedGameCount{0};
    std::size_t resumableGameCount{0};
    std::size_t recentGameCount{0};
    std::size_t activeDownloadCount{0};
    std::size_t localCaptureCount{0};
};

enum class LibraryExperienceMode {
    Empty,
    Browsing
};

enum class GameDetailPrimaryAction {
    Play,
    Resume,
    Blocked
};

struct CoreShellExperienceState {
    HomeExperienceMode homeMode{HomeExperienceMode::Empty};
    LibraryExperienceMode libraryMode{LibraryExperienceMode::Empty};
    std::size_t selectedGame{0};
};

inline constexpr HomeExperienceMode HomeModeFor(std::size_t gameCount) noexcept {
    return gameCount == 0 ? HomeExperienceMode::Empty : HomeExperienceMode::FeaturedGame;
}

inline constexpr LibraryExperienceMode LibraryModeFor(std::size_t gameCount) noexcept {
    return gameCount == 0 ? LibraryExperienceMode::Empty : LibraryExperienceMode::Browsing;
}

inline constexpr HomeLayoutDensity HomeDensityFor(float clientHeight) noexcept {
    return clientHeight < 820.0f ? HomeLayoutDensity::Compact : HomeLayoutDensity::Comfortable;
}

inline constexpr bool HomeSectionVisible(HomeSection section, const HomeDashboardSummary& summary) noexcept {
    switch (section) {
        case HomeSection::Featured:
            return summary.installedGameCount > 0;
        case HomeSection::ContinuePlaying:
            return summary.resumableGameCount > 0;
        case HomeSection::RecentlyPlayed:
            return summary.recentGameCount > 0;
        case HomeSection::InstalledGames:
            return summary.installedGameCount > 0;
        case HomeSection::Downloads:
            return summary.activeDownloadCount > 0;
        case HomeSection::Captures:
            return summary.localCaptureCount > 0;
    }
    return false;
}

inline constexpr std::size_t HomeVisibleSectionCount(const HomeDashboardSummary& summary) noexcept {
    std::size_t count = 0;
    for (HomeSection section : {HomeSection::Featured,
                                HomeSection::ContinuePlaying,
                                HomeSection::RecentlyPlayed,
                                HomeSection::InstalledGames,
                                HomeSection::Downloads,
                                HomeSection::Captures}) {
        if (HomeSectionVisible(section, summary)) ++count;
    }
    return count;
}

inline void ClampCoreShellSelection(CoreShellExperienceState& state, std::size_t gameCount) noexcept {
    state.homeMode = HomeModeFor(gameCount);
    state.libraryMode = LibraryModeFor(gameCount);
    if (gameCount == 0) {
        state.selectedGame = 0;
        return;
    }
    if (state.selectedGame >= gameCount) state.selectedGame = gameCount - 1;
}

inline bool MoveGameSelectionUp(CoreShellExperienceState& state, std::size_t gameCount) noexcept {
    ClampCoreShellSelection(state, gameCount);
    if (gameCount == 0 || state.selectedGame == 0) return false;
    --state.selectedGame;
    return true;
}

inline bool MoveGameSelectionDown(CoreShellExperienceState& state, std::size_t gameCount) noexcept {
    ClampCoreShellSelection(state, gameCount);
    if (gameCount == 0 || state.selectedGame + 1 >= gameCount) return false;
    ++state.selectedGame;
    return true;
}

inline constexpr GameDetailPrimaryAction GameDetailAction(bool launchBlocked,
                                                           bool resumeSupported,
                                                           bool resumeAvailable,
                                                           bool preferResume) noexcept {
    if (launchBlocked) return GameDetailPrimaryAction::Blocked;
    if (resumeSupported && resumeAvailable && preferResume) return GameDetailPrimaryAction::Resume;
    return GameDetailPrimaryAction::Play;
}

inline constexpr bool GameDetailCanLaunch(GameDetailPrimaryAction action) noexcept {
    return action != GameDetailPrimaryAction::Blocked;
}

inline constexpr bool GameDetailShowsResumeChoice(bool resumeSupported, bool resumeAvailable) noexcept {
    return resumeSupported && resumeAvailable;
}

} // namespace zero
