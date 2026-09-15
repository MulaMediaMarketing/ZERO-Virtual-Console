#include "v5/ProductionShellIntegration.h"
#include <iostream>
#include <iterator>
#include <string>

using namespace zero;
using namespace zero::v5;

int main() {
    ProductionShellIntegration shell;
    if (!shell.Valid()) return 1;

    std::string error;
    if (!shell.Navigate(ProductionUxPage::Library, error)) return 2;
    if (shell.Snapshot().activePage != ShellPage::Library) return 3;
    if (shell.ActivePage() != ProductionUxPage::Library) return 4;

    error.clear();
    if (!shell.Navigate(ProductionUxPage::GameDetail, error)) return 5;
    const auto detail = shell.Snapshot();
    if (detail.activePage != ShellPage::GameDetail || detail.contextualParent != ShellPage::Library) return 6;

    error.clear();
    if (!shell.Back(error)) return 7;
    if (shell.Snapshot().activePage != ShellPage::Library) return 8;

    error.clear();
    if (!shell.Navigate(ProductionUxPage::Achievements, error)) return 9;
    if (shell.Snapshot().activePage != ShellPage::Achievements) return 10;

    if (ProductionShellIntegration::ToProductionPage(ShellPage::Discover) != ProductionUxPage::Discover) return 11;
    if (ProductionShellIntegration::ToProductionPage(ShellPage::CloudPlay) != ProductionUxPage::CloudPlay) return 12;
    if (ProductionShellIntegration::ToProductionPage(ShellPage::Downloads) != ProductionUxPage::Downloads) return 13;
    if (ProductionShellIntegration::ToProductionPage(ShellPage::Profile) != ProductionUxPage::Profile) return 14;
    if (ProductionShellIntegration::ToProductionPage(ShellPage::Devices) != ProductionUxPage::Devices) return 15;

    // These destinations are now renderer-integrated and must remain navigable
    // even when their backing online service is disconnected. Availability means
    // the page can render; status communicates truthful disconnected/empty state.
    error.clear();
    if (!shell.Navigate(ProductionUxPage::Discover, error)) return 16;
    if (shell.ActivePage() != ProductionUxPage::Discover) return 17;

    bool discoverIntegrated = false;
    bool cloudIntegrated = false;
    bool downloadsIntegrated = false;
    bool profileIntegrated = false;
    bool devicesIntegrated = false;
    for (const auto& page : shell.Snapshot().pages) {
        if (page.page == ShellPage::Discover)
            discoverIntegrated = page.available && !page.status.empty();
        else if (page.page == ShellPage::CloudPlay)
            cloudIntegrated = page.available && !page.status.empty();
        else if (page.page == ShellPage::Downloads)
            downloadsIntegrated = page.available && !page.status.empty();
        else if (page.page == ShellPage::Profile)
            profileIntegrated = page.available && !page.status.empty();
        else if (page.page == ShellPage::Devices)
            devicesIntegrated = page.available && !page.status.empty();
    }
    if (!discoverIntegrated) return 18;
    if (!cloudIntegrated) return 19;
    if (!downloadsIntegrated) return 20;
    if (!profileIntegrated) return 21;
    if (!devicesIntegrated) return 22;

    if (ProductionShellIntegration::ToShellPage(ProductionUxPage::Captures) != ShellPage::Capture) return 23;
    if (ProductionShellIntegration::ToShellPage(ProductionUxPage::Import) != ShellPage::Import) return 24;
    if (ProductionShellIntegration::ToShellPage(ProductionUxPage::Notifications) != ShellPage::Notifications) return 25;
    if (ProductionShellIntegration::ToProductionPage(ShellPage::Settings) != ProductionUxPage::Settings) return 26;

    if (ProductionUxNavCount() != std::size(kTopLevelPages)) return 27;
    for (std::size_t i = 0; i < ProductionUxNavCount(); ++i) {
        const auto shellPage = ProductionShellIntegration::ToShellPage(ProductionUxPageAt(i));
        if (!shellPage || *shellPage != kTopLevelPages[i]) return 28;
    }

    // All permanent destinations are integrated. Traversal must visit them in
    // the locked V5 order instead of skipping pages whose services are offline.
    error.clear();
    if (!shell.Navigate(ProductionUxPage::Home, error)) return 29;
    if (shell.ActiveTopLevelIndex() != ProductionUxIndex(ProductionUxDestination::Home)) return 30;
    if (!shell.MoveTopLevel(1, error)) return 31;
    if (shell.ActivePage() != ProductionUxPage::Discover) return 32;
    if (!shell.MoveTopLevel(1, error)) return 33;
    if (shell.ActivePage() != ProductionUxPage::Store) return 34;
    if (!shell.Navigate(ProductionUxPage::Library, error)) return 35;
    if (!shell.MoveTopLevel(1, error)) return 36;
    if (shell.ActivePage() != ProductionUxPage::CloudPlay) return 37;
    if (!shell.MoveTopLevel(1, error)) return 38;
    if (shell.ActivePage() != ProductionUxPage::Downloads) return 39;
    if (!shell.MoveTopLevel(-1, error)) return 40;
    if (shell.ActivePage() != ProductionUxPage::CloudPlay) return 41;
    if (!shell.MoveTopLevel(-1, error)) return 42;
    if (shell.ActivePage() != ProductionUxPage::Library) return 43;

    std::cout << "ZERO V5 production UX shell integration acceptance: PASS\n";
    return 0;
}
