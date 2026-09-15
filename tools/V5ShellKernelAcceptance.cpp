#include "v5/ShellKernel.h"
#include <array>
#include <iostream>
#include <string>
#include <vector>

using namespace zero::v5;

namespace {
class Controller final : public IShellPageController {
public:
    explicit Controller(ShellPage page, bool available = true) : page_(page), available_(available) {}

    ShellPage Page() const noexcept override { return page_; }
    PageSnapshot Snapshot() const override {
        return PageSnapshot{page_, revision_, false, available_, status_};
    }
    bool Execute(const ShellCommand& command, std::string& error) override {
        if (!available_) {
            error = status_.empty() ? "unavailable" : status_;
            return false;
        }
        last_ = command.type;
        ++revision_;
        return true;
    }

    ShellCommandType Last() const noexcept { return last_; }

private:
    ShellPage page_;
    bool available_{true};
    std::string status_;
    std::uint64_t revision_{0};
    ShellCommandType last_{ShellCommandType::Activate};
};
}

int main() {
    static_assert(std::size(kTopLevelPages) == 12);

    Controller home(ShellPage::Home);
    Controller discover(ShellPage::Discover);
    Controller store(ShellPage::Store);
    Controller library(ShellPage::Library);
    Controller cloud(ShellPage::CloudPlay);
    Controller downloads(ShellPage::Downloads);
    Controller friends(ShellPage::Friends);
    Controller achievements(ShellPage::Achievements);
    Controller capture(ShellPage::Capture);
    Controller profile(ShellPage::Profile);
    Controller devices(ShellPage::Devices);
    Controller settings(ShellPage::Settings);
    Controller detail(ShellPage::GameDetail);

    std::array<IShellPageController*, 13> controllers{
        &home, &discover, &store, &library, &cloud, &downloads,
        &friends, &achievements, &capture, &profile, &devices, &settings, &detail
    };
    ShellKernel shell(controllers);
    if (!shell.Valid()) return 10;

    std::string error;
    if (!shell.Navigate(ShellPage::Discover, error)) return 11;
    if (shell.Snapshot().activePage != ShellPage::Discover) return 12;

    ShellCommand search{ShellCommandType::Search, ShellPage::Discover, "free games"};
    if (!shell.Dispatch(search, error) || discover.Last() != ShellCommandType::Search) return 13;

    if (!shell.Navigate(ShellPage::GameDetail, error)) return 14;
    auto snapshot = shell.Snapshot();
    if (snapshot.activePage != ShellPage::GameDetail ||
        !snapshot.contextualParent || *snapshot.contextualParent != ShellPage::Discover) return 15;
    if (shell.ActiveTopLevelIndex() != 1) return 16;

    if (!shell.Back(error) || shell.Snapshot().activePage != ShellPage::Discover) return 17;
    if (!shell.Back(error) || shell.Snapshot().activePage != ShellPage::Home) return 18;

    // The kernel itself owns traversal across truthful fail-closed destinations.
    // This prevents presentation code from carrying a second navigation table
    // or deciding which unavailable destinations should be skipped.
    Controller unavailableDiscover(ShellPage::Discover, false);
    Controller unavailableCloud(ShellPage::CloudPlay, false);
    Controller unavailableDownloads(ShellPage::Downloads, false);
    Controller unavailableProfile(ShellPage::Profile, false);
    Controller unavailableDevices(ShellPage::Devices, false);
    std::array<IShellPageController*, 13> productionLike{
        &home, &unavailableDiscover, &store, &library, &unavailableCloud, &unavailableDownloads,
        &friends, &achievements, &capture, &unavailableProfile, &unavailableDevices, &settings, &detail
    };
    ShellKernel traversal(productionLike);
    if (!traversal.Valid()) return 19;
    if (!traversal.MoveTopLevel(1, error) || traversal.Snapshot().activePage != ShellPage::Store) return 20;
    if (!traversal.Navigate(ShellPage::Library, error)) return 21;
    if (!traversal.MoveTopLevel(1, error) || traversal.Snapshot().activePage != ShellPage::Friends) return 22;
    if (!traversal.MoveTopLevel(-1, error) || traversal.Snapshot().activePage != ShellPage::Library) return 23;

    std::array<IShellPageController*, 11> incomplete{
        &home, &discover, &store, &library, &cloud, &downloads,
        &friends, &achievements, &capture, &profile, &devices
    };
    ShellKernel missing(incomplete);
    if (missing.Valid()) return 30;

    std::array<IShellPageController*, 13> duplicate{
        &home, &discover, &store, &library, &cloud, &downloads,
        &friends, &achievements, &capture, &profile, &devices, &settings, &settings
    };
    ShellKernel duplicates(duplicate);
    if (duplicates.Valid()) return 31;

    std::cout << "ZERO V5 shell kernel + controller architecture acceptance: PASS\n";
    return 0;
}
