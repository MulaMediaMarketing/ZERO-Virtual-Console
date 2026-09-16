#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <numeric>
#include <string>
#include <string_view>
#include <vector>

namespace zero {

enum class LibraryDisplayMode : std::uint8_t { Grid, List };
enum class LibraryFilter : std::uint8_t { All, Installed, Updates, RepairRequired, Favorites };
enum class LibrarySort : std::uint8_t { Title, LastPlayed, Playtime };

struct LibraryItemView {
    std::string title;
    bool installed{true};
    bool updateAvailable{false};
    bool repairRequired{false};
    bool favorite{false};
    std::uint64_t lastPlayedEpochSeconds{0};
    std::uint64_t totalPlaytimeSeconds{0};
};

struct LibraryBrowseState {
    LibraryDisplayMode displayMode{LibraryDisplayMode::Grid};
    LibraryFilter filter{LibraryFilter::All};
    LibrarySort sort{LibrarySort::LastPlayed};
    std::string search;
    std::size_t selectedVisibleIndex{0};
    std::size_t scrollOffset{0};
};

inline bool LibraryMatchesFilter(const LibraryItemView& item, LibraryFilter filter) noexcept {
    switch (filter) {
        case LibraryFilter::All: return true;
        case LibraryFilter::Installed: return item.installed;
        case LibraryFilter::Updates: return item.updateAvailable;
        case LibraryFilter::RepairRequired: return item.repairRequired;
        case LibraryFilter::Favorites: return item.favorite;
    }
    return false;
}

inline std::string LibraryLower(std::string_view value) {
    std::string out(value);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

inline bool LibraryMatchesSearch(const LibraryItemView& item, std::string_view search) {
    if (search.empty()) return true;
    return LibraryLower(item.title).find(LibraryLower(search)) != std::string::npos;
}

inline std::vector<std::size_t> BuildLibraryView(const std::vector<LibraryItemView>& items,
                                                  const LibraryBrowseState& state) {
    std::vector<std::size_t> view;
    view.reserve(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (LibraryMatchesFilter(items[i], state.filter) && LibraryMatchesSearch(items[i], state.search))
            view.push_back(i);
    }

    std::stable_sort(view.begin(), view.end(), [&](std::size_t lhs, std::size_t rhs) {
        const auto& a = items[lhs];
        const auto& b = items[rhs];
        switch (state.sort) {
            case LibrarySort::Title:
                return LibraryLower(a.title) < LibraryLower(b.title);
            case LibrarySort::LastPlayed:
                if (a.lastPlayedEpochSeconds != b.lastPlayedEpochSeconds)
                    return a.lastPlayedEpochSeconds > b.lastPlayedEpochSeconds;
                break;
            case LibrarySort::Playtime:
                if (a.totalPlaytimeSeconds != b.totalPlaytimeSeconds)
                    return a.totalPlaytimeSeconds > b.totalPlaytimeSeconds;
                break;
        }
        return LibraryLower(a.title) < LibraryLower(b.title);
    });
    return view;
}

inline void ClampLibraryBrowseState(LibraryBrowseState& state,
                                    std::size_t visibleCount,
                                    std::size_t pageSize) noexcept {
    if (visibleCount == 0) {
        state.selectedVisibleIndex = 0;
        state.scrollOffset = 0;
        return;
    }
    if (state.selectedVisibleIndex >= visibleCount) state.selectedVisibleIndex = visibleCount - 1;
    if (state.scrollOffset > state.selectedVisibleIndex) state.scrollOffset = state.selectedVisibleIndex;
    if (pageSize == 0) pageSize = 1;
    if (state.selectedVisibleIndex >= state.scrollOffset + pageSize)
        state.scrollOffset = state.selectedVisibleIndex - pageSize + 1;
    const std::size_t maxOffset = visibleCount > pageSize ? visibleCount - pageSize : 0;
    if (state.scrollOffset > maxOffset) state.scrollOffset = maxOffset;
}

} // namespace zero
