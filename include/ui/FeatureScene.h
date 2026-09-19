#pragma once

#include <string>
#include <utility>
#include <vector>

namespace zero::ui {

struct Rect {
    float left{};
    float top{};
    float right{};
    float bottom{};
};

enum class TextRole {
    Primary,
    Muted,
    Accent,
    Danger
};

enum class SurfaceRole {
    Card,
    Accent,
    Subtle
};

struct TextElement {
    std::wstring text;
    Rect bounds;
    bool heading{false};
    TextRole role{TextRole::Primary};
};

struct PanelElement {
    Rect bounds;
    float radius{16.0f};
    SurfaceRole role{SurfaceRole::Card};
    bool focused{false};
};

struct FeatureScene {
    std::vector<PanelElement> panels;
    std::vector<TextElement> text;

    void AddPanel(Rect bounds,
                  float radius = 16.0f,
                  SurfaceRole role = SurfaceRole::Card,
                  bool focused = false) {
        panels.push_back(PanelElement{bounds, radius, role, focused});
    }

    void AddText(std::wstring value,
                 Rect bounds,
                 bool heading = false,
                 TextRole role = TextRole::Primary) {
        text.push_back(TextElement{std::move(value), bounds, heading, role});
    }
};

} // namespace zero::ui
