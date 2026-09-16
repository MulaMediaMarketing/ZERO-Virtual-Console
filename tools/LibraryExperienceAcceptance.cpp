#include "LibraryExperience.h"
#include <iostream>
#include <string_view>
#include <vector>

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

    std::vector<LibraryItemView> items{
        {"Alpha", true, false, false, true, 100, 3600},
        {"Bravo", true, true, false, false, 400, 7200},
        {"Charlie", true, false, true, false, 300, 1800},
        {"Delta", false, false, false, true, 0, 0}
    };

    LibraryBrowseState state{};
    state.sort = LibrarySort::Title;
    auto view = BuildLibraryView(items, state);
    ok &= expect(view.size() == 4, "All filter must retain every projected library item");
    ok &= expect(items[view.front()].title == "Alpha", "title sort must be deterministic");

    state.filter = LibraryFilter::Updates;
    view = BuildLibraryView(items, state);
    ok &= expect(view.size() == 1 && items[view.front()].title == "Bravo",
        "Updates filter must expose only real update-available entries");

    state.filter = LibraryFilter::RepairRequired;
    view = BuildLibraryView(items, state);
    ok &= expect(view.size() == 1 && items[view.front()].title == "Charlie",
        "Repair filter must expose only real repair-required entries");

    state.filter = LibraryFilter::Favorites;
    view = BuildLibraryView(items, state);
    ok &= expect(view.size() == 2, "Favorites filter must use persisted favorite state only");

    state.filter = LibraryFilter::All;
    state.search = "rav";
    view = BuildLibraryView(items, state);
    ok &= expect(view.size() == 1 && items[view.front()].title == "Bravo",
        "case-insensitive search must filter the real local library");

    state.search.clear();
    state.sort = LibrarySort::LastPlayed;
    view = BuildLibraryView(items, state);
    ok &= expect(items[view.front()].title == "Bravo", "recent-play sort must place the newest real activity first");

    state.sort = LibrarySort::Playtime;
    view = BuildLibraryView(items, state);
    ok &= expect(items[view.front()].title == "Bravo", "playtime sort must use persisted playtime totals");

    state.selectedVisibleIndex = 99;
    state.scrollOffset = 90;
    ClampLibraryBrowseState(state, 4, 2);
    ok &= expect(state.selectedVisibleIndex == 3, "library selection must clamp after results shrink");
    ok &= expect(state.scrollOffset == 2, "library scroll window must remain within visible results");

    ClampLibraryBrowseState(state, 0, 2);
    ok &= expect(state.selectedVisibleIndex == 0 && state.scrollOffset == 0,
        "empty filtered results must reset selection and scroll safely");

    if (!ok) return 1;
    std::cout << "PASS: ZERO Library browse/search/filter/sort experience\n";
    return 0;
}
