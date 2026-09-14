#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace zero {

enum class ProductionUxDestination : std::size_t {
    Home = 0,
    Library,
    Store,
    Friends,
    Captures,
    Settings,
    Count
};

struct ProductionUxNavItem {
    ProductionUxDestination destination;
    std::wstring_view label;
};

inline constexpr std::array<ProductionUxNavItem, static_cast<std::size_t>(ProductionUxDestination::Count)> kProductionUxNavigation{{
    {ProductionUxDestination::Home, L"Home"},
    {ProductionUxDestination::Library, L"Library"},
    {ProductionUxDestination::Store, L"Store"},
    {ProductionUxDestination::Friends, L"Friends"},
    {ProductionUxDestination::Captures, L"Captures"},
    {ProductionUxDestination::Settings, L"Settings"},
}};

inline constexpr std::size_t ProductionUxNavCount() noexcept {
    return kProductionUxNavigation.size();
}

inline constexpr std::size_t ProductionUxIndex(ProductionUxDestination destination) noexcept {
    return static_cast<std::size_t>(destination);
}

inline constexpr ProductionUxDestination ProductionUxDestinationAt(std::size_t index) noexcept {
    return kProductionUxNavigation[index < kProductionUxNavigation.size() ? index : 0].destination;
}

inline constexpr std::size_t ProductionUxMoveLeft(std::size_t index) noexcept {
    return index == 0 ? 0 : index - 1;
}

inline constexpr std::size_t ProductionUxMoveRight(std::size_t index) noexcept {
    const auto last = kProductionUxNavigation.size() - 1;
    return index >= last ? last : index + 1;
}

inline constexpr bool ProductionUxIsPermanentDestination(ProductionUxDestination destination) noexcept {
    switch (destination) {
        case ProductionUxDestination::Home:
        case ProductionUxDestination::Library:
        case ProductionUxDestination::Store:
        case ProductionUxDestination::Friends:
        case ProductionUxDestination::Captures:
        case ProductionUxDestination::Settings:
            return true;
        default:
            return false;
    }
}

static_assert(ProductionUxNavCount() == 6, "ZERO production shell must expose six permanent top-level destinations.");
static_assert(ProductionUxIndex(ProductionUxDestination::Friends) == 3, "Friends must remain a first-class top-level destination.");
static_assert(ProductionUxIndex(ProductionUxDestination::Captures) == 4, "Captures must remain a first-class top-level destination.");

} // namespace zero
