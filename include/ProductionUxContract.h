#pragma once

#include <array>
#include <cstddef>
#include <optional>
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

enum class ProductionUxPage : std::size_t {
    Home = static_cast<std::size_t>(ProductionUxDestination::Home),
    Library = static_cast<std::size_t>(ProductionUxDestination::Library),
    Store = static_cast<std::size_t>(ProductionUxDestination::Store),
    Friends = static_cast<std::size_t>(ProductionUxDestination::Friends),
    Captures = static_cast<std::size_t>(ProductionUxDestination::Captures),
    Settings = static_cast<std::size_t>(ProductionUxDestination::Settings),
    GameDetail = static_cast<std::size_t>(ProductionUxDestination::Count),
    Import,
    Achievements
};

struct ProductionUxNavItem {
    ProductionUxDestination destination;
    ProductionUxPage page;
    std::wstring_view label;
};

inline constexpr std::array<ProductionUxNavItem, static_cast<std::size_t>(ProductionUxDestination::Count)> kProductionUxNavigation{{
    {ProductionUxDestination::Home, ProductionUxPage::Home, L"Home"},
    {ProductionUxDestination::Library, ProductionUxPage::Library, L"Library"},
    {ProductionUxDestination::Store, ProductionUxPage::Store, L"Store"},
    {ProductionUxDestination::Friends, ProductionUxPage::Friends, L"Friends"},
    {ProductionUxDestination::Captures, ProductionUxPage::Captures, L"Captures"},
    {ProductionUxDestination::Settings, ProductionUxPage::Settings, L"Settings"},
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

inline constexpr ProductionUxPage ProductionUxPageAt(std::size_t index) noexcept {
    return kProductionUxNavigation[index < kProductionUxNavigation.size() ? index : 0].page;
}

inline constexpr ProductionUxPage ProductionUxPageForDestination(ProductionUxDestination destination) noexcept {
    return ProductionUxPageAt(ProductionUxIndex(destination));
}

inline constexpr bool ProductionUxIsTopLevelPage(ProductionUxPage page) noexcept {
    return static_cast<std::size_t>(page) < ProductionUxNavCount();
}

inline constexpr std::optional<ProductionUxDestination> ProductionUxDestinationForPage(ProductionUxPage page) noexcept {
    if (!ProductionUxIsTopLevelPage(page)) return std::nullopt;
    return ProductionUxDestinationAt(static_cast<std::size_t>(page));
}

inline constexpr std::size_t ProductionUxNavIndexForPage(ProductionUxPage page) noexcept {
    const auto destination = ProductionUxDestinationForPage(page);
    return destination ? ProductionUxIndex(*destination) : ProductionUxIndex(ProductionUxDestination::Home);
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
static_assert(ProductionUxPageForDestination(ProductionUxDestination::Home) == ProductionUxPage::Home);
static_assert(ProductionUxPageForDestination(ProductionUxDestination::Friends) == ProductionUxPage::Friends);
static_assert(ProductionUxPageForDestination(ProductionUxDestination::Captures) == ProductionUxPage::Captures);
static_assert(!ProductionUxIsTopLevelPage(ProductionUxPage::GameDetail));
static_assert(!ProductionUxIsTopLevelPage(ProductionUxPage::Import));
static_assert(!ProductionUxIsTopLevelPage(ProductionUxPage::Achievements));

} // namespace zero
