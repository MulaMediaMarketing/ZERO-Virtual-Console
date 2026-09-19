#include "features/capture/CaptureFeature.h"
#include <algorithm>

namespace zero::features::capture {

CaptureViewModel CaptureController::Build(const CaptureLibrary& library,
                                          const CaptureExperienceState& state) {
    CaptureViewModel model;
    model.mode = state.mode;
    const auto& items = library.Items();
    model.selectedIndex = items.empty() ? 0 : std::min(state.selectedIndex, items.size() - 1);
    model.items.reserve(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        model.items.push_back(CaptureItemViewModel{
            item.path,
            item.kind,
            item.sizeBytes,
            i == model.selectedIndex,
            CaptureCanViewInline(item),
            CaptureCanOpenExternally(item)
        });
    }
    return model;
}

bool CaptureController::MoveSelection(CaptureViewModel& model, int direction) noexcept {
    if (model.items.empty() || direction == 0) return false;
    const auto before = model.selectedIndex;
    if (direction < 0 && model.selectedIndex > 0) --model.selectedIndex;
    if (direction > 0 && model.selectedIndex + 1 < model.items.size()) ++model.selectedIndex;
    if (before == model.selectedIndex) return false;
    for (std::size_t i = 0; i < model.items.size(); ++i) model.items[i].selected = i == model.selectedIndex;
    return true;
}

std::string CaptureView::KindLabel(CaptureKind kind) {
    return kind == CaptureKind::Screenshot ? "SCREENSHOT" : "VIDEO";
}

} // namespace zero::features::capture
