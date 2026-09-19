#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

namespace zero {

enum class ProductionUxDestination : std::size_t {
    Home = 0,
    Discover,
    Store,
    Library,
    CloudPlay,
    Downloads,
    Friends,
    Achievements,
    Captures,
    Profile,
    Devices,
    Settings,
    Count
};

enum class ProductionUxPage : std::size_t {
    Home = static_cast<std::size_t>(ProductionUxDestination::Home),
    Discover = static_cast<std::size_t>(ProductionUxDestination::Discover),
    Store = static_cast<std::size_t>(ProductionUxDestination::Store),
    Library = static_cast<std::size_t>(ProductionUxDestination::Library),
    CloudPlay = static_cast<std::size_t>(ProductionUxDestination::CloudPlay),
    Downloads = static_cast<std::size_t>(ProductionUxDestination::Downloads),
    Friends = static_cast<std::size_t>(ProductionUxDestination::Friends),
    Achievements = static_cast<std::size_t>(ProductionUxDestination::Achievements),
    Captures = static_cast<std::size_t>(ProductionUxDestination::Captures),
    Profile = static_cast<std::size_t>(ProductionUxDestination::Profile),
    Devices = static_cast<std::size_t>(ProductionUxDestination::Devices),
    Settings = static_cast<std::size_t>(ProductionUxDestination::Settings),
    GameDetail = static_cast<std::size_t>(ProductionUxDestination::Count),
    Wishlist,
    Checkout,
    Notifications,
    Import
};

struct ProductionUxNavItem {
    ProductionUxDestination destination;
    ProductionUxPage page;
    std::wstring_view label;
};

inline constexpr std::array<ProductionUxNavItem, static_cast<std::size_t>(ProductionUxDestination::Count)> kProductionUxNavigation{{
    {ProductionUxDestination::Home, ProductionUxPage::Home, L"Home"},
    {ProductionUxDestination::Discover, ProductionUxPage::Discover, L"Discover"},
    {ProductionUxDestination::Store, ProductionUxPage::Store, L"Store"},
    {ProductionUxDestination::Library, ProductionUxPage::Library, L"Library"},
    {ProductionUxDestination::CloudPlay, ProductionUxPage::CloudPlay, L"Cloud Play"},
    {ProductionUxDestination::Downloads, ProductionUxPage::Downloads, L"Downloads"},
    {ProductionUxDestination::Friends, ProductionUxPage::Friends, L"Friends"},
    {ProductionUxDestination::Achievements, ProductionUxPage::Achievements, L"Achievements"},
    {ProductionUxDestination::Captures, ProductionUxPage::Captures, L"Capture"},
    {ProductionUxDestination::Profile, ProductionUxPage::Profile, L"Profile"},
    {ProductionUxDestination::Devices, ProductionUxPage::Devices, L"Devices"},
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
    return static_cast<std::size_t>(destination) < static_cast<std::size_t>(ProductionUxDestination::Count);
}

static_assert(ProductionUxNavCount() == 12, "ZERO production shell must expose all twelve permanent V5 destinations.");
static_assert(ProductionUxIndex(ProductionUxDestination::Discover) == 1);
static_assert(ProductionUxIndex(ProductionUxDestination::Library) == 3);
static_assert(ProductionUxIndex(ProductionUxDestination::Friends) == 6);
static_assert(ProductionUxIndex(ProductionUxDestination::Achievements) == 7);
static_assert(ProductionUxIndex(ProductionUxDestination::Captures) == 8);
static_assert(ProductionUxIndex(ProductionUxDestination::Settings) == 11);
static_assert(ProductionUxIsTopLevelPage(ProductionUxPage::Achievements));
static_assert(!ProductionUxIsTopLevelPage(ProductionUxPage::GameDetail));
static_assert(!ProductionUxIsTopLevelPage(ProductionUxPage::Wishlist));
static_assert(!ProductionUxIsTopLevelPage(ProductionUxPage::Checkout));
static_assert(!ProductionUxIsTopLevelPage(ProductionUxPage::Notifications));
static_assert(!ProductionUxIsTopLevelPage(ProductionUxPage::Import));

} // namespace zero
