#pragma once

#include "ProductionUxContract.h"
#include "v5/ShellKernel.h"
#include <array>
#include <optional>
#include <string>

namespace zero::v5 {

class ProductionShellPageController final : public IShellPageController {
public:
    ProductionShellPageController(ShellPage page, bool available, std::string status = {});

    ShellPage Page() const noexcept override { return page_; }
    PageSnapshot Snapshot() const override;
    bool Execute(const ShellCommand& command, std::string& error) override;

    void SetAvailable(bool available, std::string status = {});

private:
    ShellPage page_{ShellPage::Home};
    bool available_{true};
    std::string status_;
    std::uint64_t revision_{0};
};

class ProductionShellIntegration {
public:
    ProductionShellIntegration();

    bool Valid() const noexcept { return kernel_.Valid(); }
    const std::string& CompositionError() const noexcept { return kernel_.CompositionError(); }

    bool Navigate(ProductionUxPage page, std::string& error);
    bool Back(std::string& error);
    bool Dispatch(const ShellCommand& command, std::string& error) { return kernel_.Dispatch(command, error); }
    std::optional<ProductionUxPage> ActivePage() const noexcept;
    ShellSnapshot Snapshot() const { return kernel_.Snapshot(); }

    static std::optional<ShellPage> ToShellPage(ProductionUxPage page) noexcept;
    static std::optional<ProductionUxPage> ToProductionPage(ShellPage page) noexcept;

private:
    std::array<ProductionShellPageController, 17> controllers_;
    std::array<IShellPageController*, 17> controllerRefs_{};
    ShellKernel kernel_;
};

} // namespace zero::v5
