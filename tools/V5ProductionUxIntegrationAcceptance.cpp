#include "v5/ProductionShellIntegration.h"
#include <iostream>
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

    const auto before = shell.Snapshot().activePage;
    error.clear();
    if (shell.Navigate(ProductionUxPage::Discover, error)) return 16;
    if (error.empty()) return 17;
    if (shell.Snapshot().activePage != before) return 18;

    bool discoverUnavailable = false;
    for (const auto& page : shell.Snapshot().pages) {
        if (page.page == ShellPage::Discover) {
            discoverUnavailable = !page.available && !page.status.empty();
            break;
        }
    }
    if (!discoverUnavailable) return 19;

    if (ProductionShellIntegration::ToShellPage(ProductionUxPage::Captures) != ShellPage::Capture) return 20;
    if (ProductionShellIntegration::ToShellPage(ProductionUxPage::Import) != ShellPage::Import) return 21;
    if (ProductionShellIntegration::ToShellPage(ProductionUxPage::Notifications) != ShellPage::Notifications) return 22;
    if (ProductionShellIntegration::ToProductionPage(ShellPage::Settings) != ProductionUxPage::Settings) return 23;

    if (ProductionUxNavCount() != std::size(kTopLevelPages)) return 24;
    for (std::size_t i = 0; i < ProductionUxNavCount(); ++i) {
        const auto shellPage = ProductionShellIntegration::ToShellPage(ProductionUxPageAt(i));
        if (!shellPage || *shellPage != kTopLevelPages[i]) return 25;
    }

    std::cout << "ZERO V5 production UX shell integration acceptance: PASS\n";
    return 0;
}
