#pragma once
#include <string>
#include <vector>

namespace zero {

enum class StoreProviderState {
    Disconnected,
    Ready,
    Error
};

enum class EntitlementState {
    Unknown,
    NotOwned,
    Owned
};

struct StoreProduct {
    std::string productId;
    std::string packageId;
    std::string title;
    std::string version;
    std::string shortDescription;
    std::string displayPrice;
    EntitlementState entitlement{EntitlementState::Unknown};
};

class IStoreProvider {
public:
    virtual ~IStoreProvider() = default;
    virtual StoreProviderState State() const noexcept = 0;
    virtual const std::vector<StoreProduct>& Products() const noexcept = 0;
    virtual void Refresh() = 0;
};

class DisconnectedStoreProvider final : public IStoreProvider {
public:
    StoreProviderState State() const noexcept override { return StoreProviderState::Disconnected; }
    const std::vector<StoreProduct>& Products() const noexcept override { return products_; }
    void Refresh() override {}

private:
    std::vector<StoreProduct> products_;
};

} // namespace zero
