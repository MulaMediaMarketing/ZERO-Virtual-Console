#include "ProductionUxContract.h"
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

    ok &= expect(ProductionUxNavCount() == 12, "production navigation must expose all twelve permanent V5 destinations");

    constexpr std::wstring_view expected[] = {
        L"Home", L"Discover", L"Store", L"Library", L"Cloud Play", L"Downloads",
        L"Friends", L"Achievements", L"Capture", L"Profile", L"Devices", L"Settings"
    };

    constexpr ProductionUxPage expectedPages[] = {
        ProductionUxPage::Home,
        ProductionUxPage::Discover,
        ProductionUxPage::Store,
        ProductionUxPage::Library,
        ProductionUxPage::CloudPlay,
        ProductionUxPage::Downloads,
        ProductionUxPage::Friends,
        ProductionUxPage::Achievements,
        ProductionUxPage::Captures,
        ProductionUxPage::Profile,
        ProductionUxPage::Devices,
        ProductionUxPage::Settings
    };

    for (std::size_t i = 0; i < ProductionUxNavCount(); ++i) {
        const auto& item = kProductionUxNavigation[i];
        ok &= expect(item.label == expected[i], "production navigation label/order changed");
        ok &= expect(item.page == expectedPages[i], "production page mapping changed");
        ok &= expect(ProductionUxIndex(item.destination) == i, "destination index no longer matches production order");
        ok &= expect(ProductionUxDestinationAt(i) == item.destination, "index-to-destination mapping is inconsistent");
        ok &= expect(ProductionUxPageAt(i) == item.page, "index-to-page mapping is inconsistent");
        ok &= expect(ProductionUxPageForDestination(item.destination) == item.page, "destination-to-page mapping is inconsistent");
        ok &= expect(ProductionUxNavIndexForPage(item.page) == i, "page-to-navigation mapping is inconsistent");
        ok &= expect(ProductionUxIsTopLevelPage(item.page), "permanent destination page is not top-level");
        ok &= expect(ProductionUxIsPermanentDestination(item.destination), "top-level destination is not marked permanent");
        const auto roundTrip = ProductionUxDestinationForPage(item.page);
        ok &= expect(roundTrip.has_value() && *roundTrip == item.destination, "page-to-destination round trip failed");
    }

    ok &= expect(ProductionUxDestinationAt(999) == ProductionUxDestination::Home, "out-of-range destination fallback must remain Home");
    ok &= expect(ProductionUxPageAt(999) == ProductionUxPage::Home, "out-of-range page fallback must remain Home");
    ok &= expect(ProductionUxMoveLeft(0) == 0, "left navigation must clamp at Home");
    ok &= expect(ProductionUxMoveRight(ProductionUxNavCount() - 1) == ProductionUxNavCount() - 1, "right navigation must clamp at Settings");
    ok &= expect(ProductionUxMoveRight(ProductionUxIndex(ProductionUxDestination::Home)) == ProductionUxIndex(ProductionUxDestination::Discover), "Home must be followed by Discover");
    ok &= expect(ProductionUxMoveRight(ProductionUxIndex(ProductionUxDestination::Store)) == ProductionUxIndex(ProductionUxDestination::Library), "Store must be followed by Library");
    ok &= expect(ProductionUxMoveRight(ProductionUxIndex(ProductionUxDestination::Friends)) == ProductionUxIndex(ProductionUxDestination::Achievements), "Friends must be followed by Achievements");
    ok &= expect(ProductionUxMoveRight(ProductionUxIndex(ProductionUxDestination::Achievements)) == ProductionUxIndex(ProductionUxDestination::Captures), "Achievements must be followed by Capture");

    ok &= expect(!ProductionUxDestinationForPage(ProductionUxPage::GameDetail).has_value(), "Game Detail must remain contextual, not permanent top navigation");
    ok &= expect(!ProductionUxDestinationForPage(ProductionUxPage::Wishlist).has_value(), "Wishlist must remain contextual, not permanent top navigation");
    ok &= expect(!ProductionUxDestinationForPage(ProductionUxPage::Checkout).has_value(), "Checkout must remain contextual, not permanent top navigation");
    ok &= expect(!ProductionUxDestinationForPage(ProductionUxPage::Notifications).has_value(), "Notifications must remain contextual, not permanent top navigation");
    ok &= expect(!ProductionUxDestinationForPage(ProductionUxPage::Import).has_value(), "Import must remain contextual, not permanent top navigation");

    if (!ok) return 1;
    std::cout << "PASS: ZERO production UX navigation and page integration\n";
    return 0;
}
