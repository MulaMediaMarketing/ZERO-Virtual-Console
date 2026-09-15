#include "v5/ShellKernel.h"
#include <array>
#include <iostream>
#include <string>
#include <vector>

using namespace zero::v5;

namespace {
class Controller final : public IShellPageController {
public:
    explicit Controller(ShellPage page, bool available = true, bool executeSucceeds = true)
        : page_(page), available_(available), executeSucceeds_(executeSucceeds) {}

    ShellPage Page() const noexcept override { return page_; }
    PageSnapshot Snapshot() const override {
        return PageSnapshot{page_, revision_, false, available_, status_};
    }
    bool Execute(const ShellCommand& command, std::string& error) override {
        if (!available_) {
            error = status_.empty() ? "unavailable" : status_;
            return false;
        }
        if (!executeSucceeds_) {
            error = "activation failed";
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
    bool executeSucceeds_{true};
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
    Controller checkout(ShellPage::Checkout);

    std::array<IShellPageController*, 14> controllers{
        &home, &discover, &store, &library, &cloud, &downloads,
        &friends, &achievements, &capture, &profile, &devices, &settings, &detail, &checkout
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

    // Nested contextual pages retain the permanent origin. Context may become
    // deeper, but it may not replace the top-level navigation parent.
    if (!shell.Navigate(ShellPage::Checkout, error)) return 17;
    snapshot = shell.Snapshot();
    if (snapshot.activePage != ShellPage::Checkout ||
        !snapshot.contextualParent || *snapshot.contextualParent != ShellPage::Discover) return 18;
    if (shell.ActiveTopLevelIndex() != 1) return 19;

    if (!shell.Back(error) || shell.Snapshot().activePage != ShellPage::Discover) return 20;
    if (!shell.Back(error) || shell.Snapshot().activePage != ShellPage::Home) return 21;

    // The kernel itself owns traversal across truthful fail-closed destinations.
    // This prevents presentation code from carrying a second navigation table
    // or deciding which unavailable destinations should be skipped.
    Controller unavailableDiscover(ShellPage::Discover, false);
    Controller unavailableCloud(ShellPage::CloudPlay, false);
    Controller unavailableDownloads(ShellPage::Downloads, false);
    Controller unavailableProfile(ShellPage::Profile, false);
    Controller unavailableDevices(ShellPage::Devices, false);
    std::array<IShellPageController*, 14> productionLike{
        &home, &unavailableDiscover, &store, &library, &unavailableCloud, &unavailableDownloads,
        &friends, &achievements, &capture, &unavailableProfile, &unavailableDevices, &settings, &detail, &checkout
    };
    ShellKernel traversal(productionLike);
    if (!traversal.Valid()) return 22;
    if (!traversal.MoveTopLevel(1, error) || traversal.Snapshot().activePage != ShellPage::Store) return 23;
    if (!traversal.Navigate(ShellPage::Library, error)) return 24;
    if (!traversal.MoveTopLevel(1, error) || traversal.Snapshot().activePage != ShellPage::Friends) return 25;
    if (!traversal.MoveTopLevel(-1, error) || traversal.Snapshot().activePage != ShellPage::Library) return 26;

    // Snapshot availability alone is not enough: controller activation can
    // still fail. The kernel must not publish a page or parent change unless
    // activation succeeds.
    Controller failingStore(ShellPage::Store, true, false);
    std::array<IShellPageController*, 14> transactionalControllers{
        &home, &discover, &failingStore, &library, &cloud, &downloads,
        &friends, &achievements, &capture, &profile, &devices, &settings, &detail, &checkout
    };
    ShellKernel transactional(transactionalControllers);
    if (!transactional.Valid()) return 27;
    const auto beforeFailure = transactional.Snapshot();
    error.clear();
    if (transactional.Navigate(ShellPage::Store, error)) return 28;
    const auto afterFailure = transactional.Snapshot();
    if (afterFailure.activePage != beforeFailure.activePage ||
        afterFailure.contextualParent != beforeFailure.contextualParent ||
        afterFailure.navigationRevision != beforeFailure.navigationRevision) return 29;

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
