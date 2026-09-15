#include "v5/ShellKernel.h"
#include <algorithm>
#include <array>
#include <iterator>
#include <unordered_set>

namespace zero::v5 {

bool ShellKernel::IsTopLevel(ShellPage page) noexcept {
    return std::find(std::begin(kTopLevelPages), std::end(kTopLevelPages), page) != std::end(kTopLevelPages);
}

std::size_t ShellKernel::TopLevelIndex(ShellPage page) noexcept {
    const auto it = std::find(std::begin(kTopLevelPages), std::end(kTopLevelPages), page);
    return it == std::end(kTopLevelPages) ? 0 : static_cast<std::size_t>(std::distance(std::begin(kTopLevelPages), it));
}

ShellKernel::ShellKernel(std::span<IShellPageController* const> controllers) {
    std::unordered_set<unsigned> seen;
    controllers_.reserve(controllers.size());
    for (auto* controller : controllers) {
        if (!controller) {
            compositionError_ = "shell composition contains a null controller";
            return;
        }
        const auto key = static_cast<unsigned>(controller->Page());
        if (!seen.insert(key).second) {
            compositionError_ = "shell composition contains duplicate page controllers";
            return;
        }
        controllers_.push_back(controller);
    }

    for (const auto page : kTopLevelPages) {
        if (!Find(page)) {
            compositionError_ = "shell composition is missing a required top-level controller";
            return;
        }
    }
    valid_ = true;
}

IShellPageController* ShellKernel::Find(ShellPage page) const noexcept {
    const auto it = std::find_if(controllers_.begin(), controllers_.end(), [page](const auto* controller) {
        return controller && controller->Page() == page;
    });
    return it == controllers_.end() ? nullptr : *it;
}

bool ShellKernel::Navigate(ShellPage page, std::string& error) {
    if (!valid_) {
        error = compositionError_;
        return false;
    }
    auto* controller = Find(page);
    if (!controller) {
        error = "shell page has no controller";
        return false;
    }
    const auto snapshot = controller->Snapshot();
    if (!snapshot.available) {
        error = snapshot.status.empty() ? "shell page is unavailable" : snapshot.status;
        return false;
    }

    if (!IsTopLevel(page) && activePage_ != page) contextualParent_ = activePage_;
    if (IsTopLevel(page)) contextualParent_.reset();
    if (activePage_ != page) {
        activePage_ = page;
        ++navigationRevision_;
    }

    ShellCommand activate;
    activate.type = ShellCommandType::Activate;
    activate.target = page;
    return controller->Execute(activate, error);
}

std::size_t ShellKernel::ActiveTopLevelIndex() const noexcept {
    return TopLevelIndex(contextualParent_.value_or(activePage_));
}

bool ShellKernel::MoveTopLevel(int direction, std::string& error) {
    if (!valid_) {
        error = compositionError_;
        return false;
    }
    if (direction == 0) return true;

    const auto current = ActiveTopLevelIndex();
    if (direction > 0) {
        for (std::size_t candidate = current + 1; candidate < std::size(kTopLevelPages); ++candidate) {
            error.clear();
            if (Navigate(kTopLevelPages[candidate], error)) return true;
        }
    } else {
        std::size_t candidate = current;
        while (candidate > 0) {
            --candidate;
            error.clear();
            if (Navigate(kTopLevelPages[candidate], error)) return true;
        }
    }

    error.clear();
    return true;
}

bool ShellKernel::Back(std::string& error) {
    if (contextualParent_) {
        const auto parent = *contextualParent_;
        contextualParent_.reset();
        return Navigate(parent, error);
    }
    if (activePage_ == ShellPage::Home) return true;
    return Navigate(ShellPage::Home, error);
}

bool ShellKernel::Dispatch(const ShellCommand& command, std::string& error) {
    if (command.type == ShellCommandType::Back) return Back(error);
    if (command.type == ShellCommandType::Activate) return Navigate(command.target, error);
    if (!valid_) {
        error = compositionError_;
        return false;
    }
    auto* controller = Find(command.target);
    if (!controller) {
        error = "shell command target has no controller";
        return false;
    }
    return controller->Execute(command, error);
}

ShellSnapshot ShellKernel::Snapshot() const {
    ShellSnapshot snapshot;
    snapshot.activePage = activePage_;
    snapshot.contextualParent = contextualParent_;
    snapshot.navigationRevision = navigationRevision_;
    snapshot.pages.reserve(controllers_.size());
    for (const auto* controller : controllers_) {
        if (controller) snapshot.pages.push_back(controller->Snapshot());
    }
    return snapshot;
}

} // namespace zero::v5
