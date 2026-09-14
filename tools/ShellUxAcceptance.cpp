#include "ShellUxState.h"
#include <chrono>
#include <iostream>

namespace {
void check(bool value, const char* name, bool& all) {
    std::cout << (value ? "[PASS] " : "[FAIL] ") << name << "\n";
    all &= value;
}
}

int main() {
    using namespace std::chrono_literals;
    bool all = true;

    zero::ShellUxState ux;
    check(!ux.OverlayRenderActive(), "overlay_initially_not_rendered", all);

    ux.SetOverlayVisible(true);
    check(ux.OverlayRequestedVisible(), "overlay_requested_open", all);
    check(ux.OverlayRenderActive(), "overlay_renders_while_opening", all);
    check(ux.OverlayOpacity() == 0.0f, "overlay_open_starts_transparent", all);

    ux.Advance(100ms);
    check(ux.OverlayOpacity() > 0.0f && ux.OverlayOpacity() < 1.0f,
          "overlay_open_transition_progresses", all);

    ux.Advance(200ms);
    check(ux.OverlayOpacity() > 0.99f, "overlay_reaches_full_opacity", all);

    ux.SetOverlayVisible(false);
    check(!ux.OverlayRequestedVisible(), "overlay_requested_closed", all);
    check(ux.OverlayClosing(), "overlay_reports_closing", all);
    check(ux.OverlayRenderActive(), "overlay_keeps_rendering_during_close", all);

    ux.Advance(80ms);
    check(ux.OverlayRenderActive(), "overlay_close_has_visible_midpoint", all);
    check(ux.OverlayOpacity() > 0.0f && ux.OverlayOpacity() < 1.0f,
          "overlay_close_fades", all);

    ux.Advance(120ms);
    check(!ux.OverlayRenderActive(), "overlay_stops_rendering_after_close", all);
    check(ux.OverlayOpacity() == 0.0f, "overlay_close_reaches_zero_opacity", all);

    ux.SetReducedMotion(true);
    ux.SetOverlayVisible(true);
    check(ux.OverlayOpacity() == 1.0f, "reduced_motion_opens_immediately", all);
    ux.SetOverlayVisible(false);
    check(!ux.OverlayRenderActive(), "reduced_motion_closes_immediately", all);
    check(ux.FocusThickness() >= 3.5f, "focus_ring_has_couch_readable_weight", all);

    ux.SetReducedMotion(false);
    ux.BeginPageTransition();
    check(ux.PageOffsetY() > 0.0f, "page_transition_has_subtle_entry_offset", all);
    ux.Advance(250ms);
    check(ux.PageOffsetY() == 0.0f, "page_transition_settles", all);

    std::cout << "Result: " << (all ? "PASS" : "FAIL") << "\n";
    return all ? 0 : 2;
}
