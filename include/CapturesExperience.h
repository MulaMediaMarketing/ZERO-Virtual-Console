#pragma once

#include "CaptureLibrary.h"
#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

namespace zero {

enum class CaptureExperienceMode {
    Empty,
    Browsing,
    Viewer,
    DeleteConfirm
};

struct CaptureExperienceState {
    CaptureExperienceMode mode{CaptureExperienceMode::Empty};
    std::size_t selectedIndex{0};
    std::size_t scrollOffset{0};
    std::size_t visibleRows{6};
};

inline constexpr bool CaptureCanViewInline(const CaptureItem& item) noexcept {
    return item.kind == CaptureKind::Screenshot;
}

inline constexpr bool CaptureCanOpenExternally(const CaptureItem& item) noexcept {
    return item.kind == CaptureKind::Video;
}

inline void ClampCaptureExperience(CaptureExperienceState& state,
                                   const std::vector<CaptureItem>& items) noexcept {
    if (state.visibleRows == 0) state.visibleRows = 1;
    if (items.empty()) {
        state.mode = CaptureExperienceMode::Empty;
        state.selectedIndex = 0;
        state.scrollOffset = 0;
        return;
    }

    if (state.mode == CaptureExperienceMode::Empty) state.mode = CaptureExperienceMode::Browsing;
    if (state.selectedIndex >= items.size()) state.selectedIndex = items.size() - 1;
    if (state.selectedIndex < state.scrollOffset) state.scrollOffset = state.selectedIndex;
    if (state.selectedIndex >= state.scrollOffset + state.visibleRows)
        state.scrollOffset = state.selectedIndex - state.visibleRows + 1;

    const std::size_t maxStart = items.size() > state.visibleRows ? items.size() - state.visibleRows : 0;
    state.scrollOffset = std::min(state.scrollOffset, maxStart);
}

inline bool MoveCaptureSelectionUp(CaptureExperienceState& state,
                                   const std::vector<CaptureItem>& items) noexcept {
    ClampCaptureExperience(state, items);
    if (state.mode != CaptureExperienceMode::Browsing || state.selectedIndex == 0) return false;
    --state.selectedIndex;
    ClampCaptureExperience(state, items);
    return true;
}

inline bool MoveCaptureSelectionDown(CaptureExperienceState& state,
                                     const std::vector<CaptureItem>& items) noexcept {
    ClampCaptureExperience(state, items);
    if (state.mode != CaptureExperienceMode::Browsing || state.selectedIndex + 1 >= items.size()) return false;
    ++state.selectedIndex;
    ClampCaptureExperience(state, items);
    return true;
}

inline std::optional<std::size_t> SelectedCaptureIndex(const CaptureExperienceState& state,
                                                       const std::vector<CaptureItem>& items) noexcept {
    if (items.empty() || state.selectedIndex >= items.size()) return std::nullopt;
    return state.selectedIndex;
}

inline bool OpenCaptureViewer(CaptureExperienceState& state,
                              const std::vector<CaptureItem>& items) noexcept {
    const auto index = SelectedCaptureIndex(state, items);
    if (!index || !CaptureCanViewInline(items[*index])) return false;
    state.mode = CaptureExperienceMode::Viewer;
    return true;
}

inline bool BeginCaptureDelete(CaptureExperienceState& state,
                               const std::vector<CaptureItem>& items) noexcept {
    if (!SelectedCaptureIndex(state, items)) return false;
    state.mode = CaptureExperienceMode::DeleteConfirm;
    return true;
}

inline void CancelCaptureModal(CaptureExperienceState& state,
                               const std::vector<CaptureItem>& items) noexcept {
    state.mode = items.empty() ? CaptureExperienceMode::Empty : CaptureExperienceMode::Browsing;
    ClampCaptureExperience(state, items);
}

inline void OnCaptureDeleted(CaptureExperienceState& state,
                             const std::vector<CaptureItem>& itemsAfterDelete) noexcept {
    state.mode = itemsAfterDelete.empty() ? CaptureExperienceMode::Empty : CaptureExperienceMode::Browsing;
    ClampCaptureExperience(state, itemsAfterDelete);
}

} // namespace zero
