#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace zero::v5 {

enum class ShellPage : std::uint8_t {
    Home,
    Discover,
    Store,
    Library,
    CloudPlay,
    Downloads,
    Friends,
    Achievements,
    Capture,
    Profile,
    Devices,
    Settings,
    GameDetail,
    Wishlist,
    Checkout,
    Notifications,
    Import
};

inline constexpr ShellPage kTopLevelPages[] = {
    ShellPage::Home,
    ShellPage::Discover,
    ShellPage::Store,
    ShellPage::Library,
    ShellPage::CloudPlay,
    ShellPage::Downloads,
    ShellPage::Friends,
    ShellPage::Achievements,
    ShellPage::Capture,
    ShellPage::Profile,
    ShellPage::Devices,
    ShellPage::Settings
};

enum class ShellCommandType : std::uint8_t {
    Activate,
    Refresh,
    Select,
    Back,
    Search,
    PrimaryAction,
    SecondaryAction
};

struct ShellCommand {
    ShellCommandType type{ShellCommandType::Activate};
    ShellPage target{ShellPage::Home};
    std::string argument;
};

struct PageSnapshot {
    ShellPage page{ShellPage::Home};
    std::uint64_t revision{0};
    bool loading{false};
    bool available{true};
    std::string status;
};

struct ShellSnapshot {
    ShellPage activePage{ShellPage::Home};
    std::optional<ShellPage> contextualParent;
    std::uint64_t navigationRevision{0};
    std::vector<PageSnapshot> pages;
};

class IShellPageController {
public:
    virtual ~IShellPageController() = default;
    virtual ShellPage Page() const noexcept = 0;
    virtual PageSnapshot Snapshot() const = 0;
    virtual bool Execute(const ShellCommand& command, std::string& error) = 0;
};

class ShellKernel {
public:
    explicit ShellKernel(std::span<IShellPageController* const> controllers);

    bool Valid() const noexcept { return valid_; }
    const std::string& CompositionError() const noexcept { return compositionError_; }

    bool Navigate(ShellPage page, std::string& error);
    bool Back(std::string& error);
    bool Dispatch(const ShellCommand& command, std::string& error);
    ShellSnapshot Snapshot() const;

private:
    std::vector<IShellPageController*> controllers_;
    ShellPage activePage_{ShellPage::Home};
    std::optional<ShellPage> contextualParent_;
    std::uint64_t navigationRevision_{0};
    bool valid_{false};
    std::string compositionError_;

    IShellPageController* Find(ShellPage page) const noexcept;
    static bool IsTopLevel(ShellPage page) noexcept;
};

} // namespace zero::v5
