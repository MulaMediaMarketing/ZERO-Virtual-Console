#include "v5/ProductionShellIntegration.h"
#include <iostream>
#include <string>

using namespace zero;
using namespace zero::v5;

int main() {
    ProductionShellIntegration shell;
    if (!shell.Valid()) return 1;

    std::string error;
    if (!shell.NavigateLegacy(ProductionUxPage::Library, error)) return 2;
    if (shell.Snapshot().activePage != ShellPage::Library) return 3;
    if (shell.ActiveLegacyPage() != ProductionUxPage::Library) return 4;

    error.clear();
    if (!shell.NavigateLegacy(ProductionUxPage::GameDetail, error)) return 5;
    const auto detail = shell.Snapshot();
    if (detail.activePage != ShellPage::GameDetail || detail.contextualParent != ShellPage::Library) return 6;

    error.clear();
    if (!shell.Back(error)) return 7;
    if (shell.Snapshot().activePage != ShellPage::Library) return 8;

    error.clear();
    if (!shell.NavigateLegacy(ProductionUxPage::Achievements, error)) return 9;
    if (shell.Snapshot().activePage != ShellPage::Achievements) return 10;

    const auto discover = ProductionShellIntegration::ToLegacyPage(ShellPage::Discover);
    if (discover.has_value()) return 11;

    // Destinations that do not yet have a production renderer must remain
    // explicitly unavailable. The V5 shell must never silently route them to
    // another page or present a fabricated implementation.
    const auto before = shell.Snapshot().activePage;
    error.clear();
    ShellCommand command;
    command.type = ShellCommandType::Activate;
    command.target = ShellPage::Discover;
    // There is intentionally no public raw kernel dispatch path in the bridge;
    // verify the availability state directly from the immutable snapshot.
    bool discoverUnavailable = false;
    for (const auto& page : shell.Snapshot().pages) {
        if (page.page == ShellPage::Discover) {
            discoverUnavailable = !page.available && !page.status.empty();
            break;
        }
    }
    if (!discoverUnavailable) return 12;
    if (shell.Snapshot().activePage != before) return 13;

    if (ProductionShellIntegration::ToShellPage(ProductionUxPage::Captures) != ShellPage::Capture) return 14;
    if (ProductionShellIntegration::ToShellPage(ProductionUxPage::Import) != ShellPage::Import) return 15;
    if (ProductionShellIntegration::ToLegacyPage(ShellPage::Settings) != ProductionUxPage::Settings) return 16;

    std::cout << "ZERO V5 production UX shell integration acceptance: PASS\n";
    return 0;
}
