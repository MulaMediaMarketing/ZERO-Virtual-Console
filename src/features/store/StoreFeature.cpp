#include "features/store/StoreFeature.h"
#include <algorithm>

namespace zero::features::store {

StoreViewModel StoreController::Build(const IStoreProvider& provider,
                                      const StoreExperienceState& state) {
    StoreViewModel model;
    model.providerState = provider.State();
    model.mode = state.mode;
    model.selectedIndex = state.selectedIndex;

    const auto& products = provider.Products();
    if (!products.empty()) model.selectedIndex = std::min(model.selectedIndex, products.size() - 1);
    else model.selectedIndex = 0;

    model.items.reserve(products.size());
    for (std::size_t i = 0; i < products.size(); ++i) {
        const auto& product = products[i];
        StoreItemViewModel item;
        item.productId = product.productId;
        item.packageId = product.packageId;
        item.title = product.title;
        item.version = product.version;
        item.description = product.shortDescription;
        item.price = product.displayPrice;
        item.owned = StoreProductIsOwned(product);
        item.purchasable = StoreProductCanLaunchPurchase(product);
        item.selected = i == model.selectedIndex;
        model.items.push_back(std::move(item));
    }
    return model;
}

bool StoreController::MoveSelection(StoreViewModel& model, int direction) noexcept {
    if (model.items.empty() || direction == 0) return false;
    const auto before = model.selectedIndex;
    if (direction < 0 && model.selectedIndex > 0) --model.selectedIndex;
    if (direction > 0 && model.selectedIndex + 1 < model.items.size()) ++model.selectedIndex;
    if (before == model.selectedIndex) return false;
    for (std::size_t i = 0; i < model.items.size(); ++i) model.items[i].selected = i == model.selectedIndex;
    return true;
}

std::string StoreView::EmptyStateTitle(const StoreViewModel& model) {
    if (model.providerState == StoreProviderState::Disconnected) return "ZERO Store is not connected";
    if (model.providerState == StoreProviderState::Error) return "ZERO Store is unavailable";
    if (model.items.empty()) return "No Store products available";
    return {};
}

} // namespace zero::features::store
