#include "CoreShellExperience.h"
#include <iostream>
#include <string_view>

namespace {
bool expect(bool condition, std::string_view message) {
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}
}

int main() {
    using namespace zero;
    bool ok = true;

    CoreShellExperienceState state{};
    ClampCoreShellSelection(state, 0);
    ok &= expect(state.homeMode == HomeExperienceMode::Empty, "Home must expose an explicit empty state");
    ok &= expect(state.libraryMode == LibraryExperienceMode::Empty, "Library must expose an explicit empty state");
    ok &= expect(state.selectedGame == 0, "empty library selection must reset to zero");

    ClampCoreShellSelection(state, 4);
    ok &= expect(state.homeMode == HomeExperienceMode::FeaturedGame, "Home must expose a featured installed game state");
    ok &= expect(state.libraryMode == LibraryExperienceMode::Browsing, "Library must expose browsing when games exist");

    HomeDashboardSummary emptyHome{};
    ok &= expect(HomeVisibleSectionCount(emptyHome) == 0, "Home must not synthesize sections when no real local data exists");
    ok &= expect(!HomeSectionVisible(HomeSection::ContinuePlaying, emptyHome), "Continue Playing must remain hidden without an authoritative resume checkpoint");
    ok &= expect(!HomeSectionVisible(HomeSection::Downloads, emptyHome), "Downloads must remain hidden without real jobs");
    ok &= expect(!HomeSectionVisible(HomeSection::Captures, emptyHome), "Captures must remain hidden without local captures");

    HomeDashboardSummary populatedHome{};
    populatedHome.installedGameCount = 4;
    populatedHome.resumableGameCount = 1;
    populatedHome.recentGameCount = 3;
    populatedHome.activeDownloadCount = 2;
    populatedHome.localCaptureCount = 5;
    ok &= expect(HomeSectionVisible(HomeSection::Featured, populatedHome), "installed games must expose a featured Home surface");
    ok &= expect(HomeSectionVisible(HomeSection::ContinuePlaying, populatedHome), "real resume data must expose Continue Playing");
    ok &= expect(HomeSectionVisible(HomeSection::RecentlyPlayed, populatedHome), "real play history must expose Recently Played");
    ok &= expect(HomeSectionVisible(HomeSection::InstalledGames, populatedHome), "installed games must expose the installed summary");
    ok &= expect(HomeSectionVisible(HomeSection::Downloads, populatedHome), "real download jobs must expose download activity");
    ok &= expect(HomeSectionVisible(HomeSection::Captures, populatedHome), "real local captures must expose capture activity");
    ok &= expect(HomeVisibleSectionCount(populatedHome) == 6, "all six Home sections must be available when backed by real local state");
    ok &= expect(HomeDensityFor(720.0f) == HomeLayoutDensity::Compact, "720p must use the compact Home layout");
    ok &= expect(HomeDensityFor(1080.0f) == HomeLayoutDensity::Comfortable, "1080p must use the comfortable Home layout");

    ok &= expect(!MoveGameSelectionUp(state, 4), "selection must clamp at the first game");
    ok &= expect(MoveGameSelectionDown(state, 4), "selection must move down when another game exists");
    ok &= expect(state.selectedGame == 1, "selection must advance deterministically");
    state.selectedGame = 99;
    ClampCoreShellSelection(state, 4);
    ok &= expect(state.selectedGame == 3, "selection must clamp after library contents shrink");

    ok &= expect(GameDetailAction(true, true, true, true) == GameDetailPrimaryAction::Blocked,
        "package trust must override Play and Resume");
    ok &= expect(GameDetailAction(false, true, true, true) == GameDetailPrimaryAction::Resume,
        "available preferred resume must resolve to Resume");
    ok &= expect(GameDetailAction(false, true, true, false) == GameDetailPrimaryAction::Play,
        "explicit Play choice must remain Play");
    ok &= expect(GameDetailAction(false, false, false, true) == GameDetailPrimaryAction::Play,
        "games without resume support must fall back to Play");
    ok &= expect(!GameDetailCanLaunch(GameDetailPrimaryAction::Blocked), "blocked packages must fail closed");
    ok &= expect(GameDetailCanLaunch(GameDetailPrimaryAction::Play), "Play must be launchable");
    ok &= expect(GameDetailShowsResumeChoice(true, true), "resume choice requires support and a checkpoint");
    ok &= expect(!GameDetailShowsResumeChoice(true, false), "resume choice must not appear without a checkpoint");

    if (!ok) return 1;
    std::cout << "PASS: ZERO Home/Library/Game Detail production UX contract\n";
    return 0;
}
