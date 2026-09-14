#pragma once

#include "FirstBootService.h"
#include "StoreProvider.h"
#include <algorithm>
#include <cstddef>
#include <string_view>

namespace zero {

enum class StoreExperienceMode {
    Disconnected,
    Error,
    Empty,
    Browsing
};

enum class SettingsExperienceRow : std::size_t {
    Profile = 0,
    Volume,
    ReducedMotion,
    Controller,
    Display,
    Storage,
    Diagnostics,
    About,
    Count
};

enum class FirstBootExperienceMode {
    Required,
    InProgress,
    Complete
};

struct StoreExperienceState {
    StoreExperienceMode mode{StoreExperienceMode::Disconnected};
    std::size_t selectedIndex{0};
    std::size_t scrollOffset{0};
    std::size_t visibleRows{6};
};

struct SettingsExperienceState {
    std::size_t selectedRow{0};
};

inline constexpr StoreExperienceMode StoreModeFor(StoreProviderState providerState,
                                                   std::size_t productCount) noexcept {
    if (providerState == StoreProviderState::Disconnected) return StoreExperienceMode::Disconnected;
    if (providerState == StoreProviderState::Error) return StoreExperienceMode::Error;
    return productCount == 0 ? StoreExperienceMode::Empty : StoreExperienceMode::Browsing;
}

inline constexpr bool StoreProductCanLaunchPurchase(const StoreProduct& product) noexcept {
    return !product.productId.empty() &&
           !product.packageId.empty() &&
           product.entitlement == EntitlementState::NotOwned;
}

inline constexpr bool StoreProductIsOwned(const StoreProduct& product) noexcept {
    return product.entitlement == EntitlementState::Owned;
}

inline void ClampStoreSelection(StoreExperienceState& state,
                                StoreProviderState providerState,
                                std::size_t productCount) noexcept {
    state.mode = StoreModeFor(providerState, productCount);
    if (state.visibleRows == 0) state.visibleRows = 1;
    if (state.mode != StoreExperienceMode::Browsing) {
        state.selectedIndex = 0;
        state.scrollOffset = 0;
        return;
    }
    if (state.selectedIndex >= productCount) state.selectedIndex = productCount - 1;
    if (state.selectedIndex < state.scrollOffset) state.scrollOffset = state.selectedIndex;
    if (state.selectedIndex >= state.scrollOffset + state.visibleRows)
        state.scrollOffset = state.selectedIndex - state.visibleRows + 1;
    const auto maxStart = productCount > state.visibleRows ? productCount - state.visibleRows : 0;
    state.scrollOffset = std::min(state.scrollOffset, maxStart);
}

inline bool MoveStoreSelectionUp(StoreExperienceState& state,
                                 StoreProviderState providerState,
                                 std::size_t productCount) noexcept {
    ClampStoreSelection(state, providerState, productCount);
    if (state.mode != StoreExperienceMode::Browsing || state.selectedIndex == 0) return false;
    --state.selectedIndex;
    ClampStoreSelection(state, providerState, productCount);
    return true;
}

inline bool MoveStoreSelectionDown(StoreExperienceState& state,
                                   StoreProviderState providerState,
                                   std::size_t productCount) noexcept {
    ClampStoreSelection(state, providerState, productCount);
    if (state.mode != StoreExperienceMode::Browsing || state.selectedIndex + 1 >= productCount) return false;
    ++state.selectedIndex;
    ClampStoreSelection(state, providerState, productCount);
    return true;
}

inline constexpr std::size_t SettingsRowCount() noexcept {
    return static_cast<std::size_t>(SettingsExperienceRow::Count);
}

inline void ClampSettingsSelection(SettingsExperienceState& state) noexcept {
    const auto last = SettingsRowCount() - 1;
    if (state.selectedRow > last) state.selectedRow = last;
}

inline bool MoveSettingsUp(SettingsExperienceState& state) noexcept {
    ClampSettingsSelection(state);
    if (state.selectedRow == 0) return false;
    --state.selectedRow;
    return true;
}

inline bool MoveSettingsDown(SettingsExperienceState& state) noexcept {
    ClampSettingsSelection(state);
    const auto last = SettingsRowCount() - 1;
    if (state.selectedRow >= last) return false;
    ++state.selectedRow;
    return true;
}

inline constexpr int ClampVolume(int volume) noexcept {
    return std::clamp(volume, 0, 100);
}

inline constexpr int AdjustVolume(int volume, int delta) noexcept {
    return ClampVolume(volume + delta);
}

inline FirstBootExperienceMode FirstBootModeFor(const FirstBootState& state) noexcept {
    if (state.completed && FirstBootRequirementsSatisfied(state)) return FirstBootExperienceMode::Complete;
    if (state.controllerConfirmed || state.displayConfirmed || state.audioConfirmed || state.profileName != "Player")
        return FirstBootExperienceMode::InProgress;
    return FirstBootExperienceMode::Required;
}

inline bool FirstBootCanComplete(const FirstBootState& state) noexcept {
    return !state.completed && FirstBootRequirementsSatisfied(state);
}

} // namespace zero
