#include "CapturesExperience.h"
#include <iostream>
#include <string_view>
#include <vector>

namespace {
bool expect(bool condition, std::string_view message) {
    if (condition) return true;
    std::cerr << "FAIL: " << message << '\n';
    return false;
}
}

int main() {
    using namespace zero;
    bool ok = true;

    CaptureExperienceState state{};
    std::vector<CaptureItem> items;
    ClampCaptureExperience(state, items);
    ok &= expect(state.mode == CaptureExperienceMode::Empty, "empty library must report Empty mode");
    ok &= expect(state.selectedIndex == 0 && state.scrollOffset == 0, "empty library must clear selection state");

    CaptureItem shot{};
    shot.kind = CaptureKind::Screenshot;
    shot.path = L"shot.png";
    CaptureItem video{};
    video.kind = CaptureKind::Video;
    video.path = L"clip.mp4";
    items = {shot, video};

    ClampCaptureExperience(state, items);
    ok &= expect(state.mode == CaptureExperienceMode::Browsing, "non-empty library must enter Browsing mode");
    ok &= expect(CaptureCanViewInline(items[0]), "screenshots must be inline-viewable");
    ok &= expect(!CaptureCanViewInline(items[1]), "videos must not enter screenshot viewer");
    ok &= expect(CaptureCanOpenExternally(items[1]), "videos must remain externally openable");

    ok &= expect(MoveCaptureSelectionDown(state, items), "selection should move down when a next item exists");
    ok &= expect(state.selectedIndex == 1, "selection must land on second capture");
    ok &= expect(!OpenCaptureViewer(state, items), "video must fail closed for inline viewer");
    ok &= expect(BeginCaptureDelete(state, items), "selected capture must enter delete confirmation");
    ok &= expect(state.mode == CaptureExperienceMode::DeleteConfirm, "delete confirmation mode must be explicit");
    CancelCaptureModal(state, items);
    ok &= expect(state.mode == CaptureExperienceMode::Browsing, "cancel must return to browsing");

    state.selectedIndex = 0;
    ok &= expect(OpenCaptureViewer(state, items), "screenshot should open inline viewer");
    ok &= expect(state.mode == CaptureExperienceMode::Viewer, "viewer mode must be explicit");
    CancelCaptureModal(state, items);

    items.erase(items.begin());
    state.selectedIndex = 7;
    ClampCaptureExperience(state, items);
    ok &= expect(state.selectedIndex == 0, "selection must clamp after library mutation");

    items.clear();
    OnCaptureDeleted(state, items);
    ok &= expect(state.mode == CaptureExperienceMode::Empty, "deleting the final capture must return to Empty mode");

    if (!ok) return 1;
    std::cout << "PASS: ZERO Captures experience\n";
    return 0;
}
