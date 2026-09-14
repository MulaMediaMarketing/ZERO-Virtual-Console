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

    ok &= expect(ProductionUxNavCount() == 6, "production navigation must expose exactly six permanent destinations");

    constexpr std::wstring_view expected[] = {
        L"Home", L"Library", L"Store", L"Friends", L"Captures", L"Settings"
    };

    for (std::size_t i = 0; i < ProductionUxNavCount(); ++i) {
        ok &= expect(kProductionUxNavigation[i].label == expected[i], "production navigation label/order changed");
        ok &= expect(ProductionUxIndex(kProductionUxNavigation[i].destination) == i, "destination index no longer matches production order");
        ok &= expect(ProductionUxDestinationAt(i) == kProductionUxNavigation[i].destination, "index-to-destination mapping is inconsistent");
        ok &= expect(ProductionUxIsPermanentDestination(kProductionUxNavigation[i].destination), "top-level destination is not marked permanent");
    }

    ok &= expect(ProductionUxDestinationAt(999) == ProductionUxDestination::Home, "out-of-range destination fallback must remain Home");
    ok &= expect(ProductionUxMoveLeft(0) == 0, "left navigation must clamp at Home");
    ok &= expect(ProductionUxMoveRight(ProductionUxNavCount() - 1) == ProductionUxNavCount() - 1, "right navigation must clamp at Settings");
    ok &= expect(ProductionUxMoveRight(ProductionUxIndex(ProductionUxDestination::Store)) == ProductionUxIndex(ProductionUxDestination::Friends), "Store must navigate directly to Friends");
    ok &= expect(ProductionUxMoveRight(ProductionUxIndex(ProductionUxDestination::Friends)) == ProductionUxIndex(ProductionUxDestination::Captures), "Friends must navigate directly to Captures");
    ok &= expect(ProductionUxMoveLeft(ProductionUxIndex(ProductionUxDestination::Captures)) == ProductionUxIndex(ProductionUxDestination::Friends), "Captures must navigate directly back to Friends");

    if (!ok) return 1;
    std::cout << "PASS: ZERO production UX navigation contract\n";
    return 0;
}
