#include "v5/ProductionShellIntegration.h"

namespace zero::v5 {

ProductionShellPageController::ProductionShellPageController(ShellPage page, bool available, std::string status)
    : page_(page), available_(available), status_(std::move(status)) {}

PageSnapshot ProductionShellPageController::Snapshot() const {
    PageSnapshot snapshot;
    snapshot.page = page_;
    snapshot.revision = revision_;
    snapshot.available = available_;
    snapshot.status = status_;
    return snapshot;
}

bool ProductionShellPageController::Execute(const ShellCommand& command, std::string& error) {
    if (command.target != page_) {
        error = "shell command was routed to the wrong page controller";
        return false;
    }
    if (!available_) {
        error = status_.empty() ? "shell page is unavailable" : status_;
        return false;
    }
    if (command.type == ShellCommandType::Activate || command.type == ShellCommandType::Refresh ||
        command.type == ShellCommandType::Select || command.type == ShellCommandType::Search ||
        command.type == ShellCommandType::PrimaryAction || command.type == ShellCommandType::SecondaryAction) {
        ++revision_;
        return true;
    }
    error = "unsupported shell command for production page controller";
    return false;
}

void ProductionShellPageController::SetAvailable(bool available, std::string status) {
    available_ = available;
    status_ = std::move(status);
    ++revision_;
}

ProductionShellIntegration::ProductionShellIntegration()
    : controllers_{{
          {ShellPage::Home, true},
          {ShellPage::Discover, true, "Local discovery available; online ZERO catalog disconnected"},
          {ShellPage::Store, true},
          {ShellPage::Library, true},
          {ShellPage::CloudPlay, true, "ZERO Cloud service disconnected"},
          {ShellPage::Downloads, true, "Local installs available; no authoritative remote transfer jobs"},
          {ShellPage::Friends, true},
          {ShellPage::Achievements, true},
          {ShellPage::Capture, true},
          {ShellPage::Profile, true, "Local ZERO identity available; online identity disconnected"},
          {ShellPage::Devices, true, "Local PC/controller state available; no authoritative paired ZERO devices"},
          {ShellPage::Settings, true},
          {ShellPage::GameDetail, true},
          {ShellPage::Wishlist, true, "ZERO commerce service disconnected"},
          {ShellPage::Checkout, true, "ZERO checkout service disconnected"},
          {ShellPage::Notifications, true, "Local system/game activity available; online notification services disconnected"},
          {ShellPage::Import, true},
      }},
      controllerRefs_{{
          &controllers_[0], &controllers_[1], &controllers_[2], &controllers_[3], &controllers_[4],
          &controllers_[5], &controllers_[6], &controllers_[7], &controllers_[8], &controllers_[9],
          &controllers_[10], &controllers_[11], &controllers_[12], &controllers_[13], &controllers_[14],
          &controllers_[15], &controllers_[16],
      }},
      kernel_(controllerRefs_) {}

std::optional<ShellPage> ProductionShellIntegration::ToShellPage(ProductionUxPage page) noexcept {
    switch (page) {
        case ProductionUxPage::Home: return ShellPage::Home;
        case ProductionUxPage::Discover: return ShellPage::Discover;
        case ProductionUxPage::Store: return ShellPage::Store;
        case ProductionUxPage::Library: return ShellPage::Library;
        case ProductionUxPage::CloudPlay: return ShellPage::CloudPlay;
        case ProductionUxPage::Downloads: return ShellPage::Downloads;
        case ProductionUxPage::Friends: return ShellPage::Friends;
        case ProductionUxPage::Achievements: return ShellPage::Achievements;
        case ProductionUxPage::Captures: return ShellPage::Capture;
        case ProductionUxPage::Profile: return ShellPage::Profile;
        case ProductionUxPage::Devices: return ShellPage::Devices;
        case ProductionUxPage::Settings: return ShellPage::Settings;
        case ProductionUxPage::GameDetail: return ShellPage::GameDetail;
        case ProductionUxPage::Wishlist: return ShellPage::Wishlist;
        case ProductionUxPage::Checkout: return ShellPage::Checkout;
        case ProductionUxPage::Notifications: return ShellPage::Notifications;
        case ProductionUxPage::Import: return ShellPage::Import;
    }
    return std::nullopt;
}

std::optional<ProductionUxPage> ProductionShellIntegration::ToProductionPage(ShellPage page) noexcept {
    switch (page) {
        case ShellPage::Home: return ProductionUxPage::Home;
        case ShellPage::Discover: return ProductionUxPage::Discover;
        case ShellPage::Store: return ProductionUxPage::Store;
        case ShellPage::Library: return ProductionUxPage::Library;
        case ShellPage::CloudPlay: return ProductionUxPage::CloudPlay;
        case ShellPage::Downloads: return ProductionUxPage::Downloads;
        case ShellPage::Friends: return ProductionUxPage::Friends;
        case ShellPage::Achievements: return ProductionUxPage::Achievements;
        case ShellPage::Capture: return ProductionUxPage::Captures;
        case ShellPage::Profile: return ProductionUxPage::Profile;
        case ShellPage::Devices: return ProductionUxPage::Devices;
        case ShellPage::Settings: return ProductionUxPage::Settings;
        case ShellPage::GameDetail: return ProductionUxPage::GameDetail;
        case ShellPage::Wishlist: return ProductionUxPage::Wishlist;
        case ShellPage::Checkout: return ProductionUxPage::Checkout;
        case ShellPage::Notifications: return ProductionUxPage::Notifications;
        case ShellPage::Import: return ProductionUxPage::Import;
    }
    return std::nullopt;
}

bool ProductionShellIntegration::Navigate(ProductionUxPage page, std::string& error) {
    const auto shellPage = ToShellPage(page);
    if (!shellPage) {
        error = "production UX page has no V5 shell mapping";
        return false;
    }
    return kernel_.Navigate(*shellPage, error);
}

bool ProductionShellIntegration::NavigateContextual(ProductionUxPage page, std::string& error) {
    const auto shellPage = ToShellPage(page);
    if (!shellPage) {
        error = "production UX page has no V5 shell mapping";
        return false;
    }
    return kernel_.NavigateContextual(*shellPage, error);
}

std::size_t ProductionShellIntegration::ActiveTopLevelIndex() const noexcept {
    return kernel_.ActiveTopLevelIndex();
}

bool ProductionShellIntegration::MoveTopLevel(int direction, std::string& error) {
    return kernel_.MoveTopLevel(direction, error);
}

bool ProductionShellIntegration::Back(std::string& error) {
    return kernel_.Back(error);
}

std::optional<ProductionUxPage> ProductionShellIntegration::ActivePage() const noexcept {
    return ToProductionPage(kernel_.Snapshot().activePage);
}

} // namespace zero::v5
