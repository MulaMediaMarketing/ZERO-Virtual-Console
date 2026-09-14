#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>

namespace zero {

enum class HomeExperienceMode {
    Empty,
    FeaturedGame
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
