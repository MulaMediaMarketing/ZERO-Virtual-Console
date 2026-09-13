#include "App.h"
#include <algorithm>

using Microsoft::WRL::ComPtr;

namespace zero {

void App::ClampAchievementSelection() {
    const auto records = achievementPackageId_.empty()
        ? std::vector<AchievementRecord>{}
        : runtime_.Achievements(achievementPackageId_);
    if (records.empty()) {
        selectedAchievement_ = 0;
        achievementScroll_ = 0;
        return;
    }
    if (selectedAchievement_ >= records.size()) selectedAchievement_ = records.size() - 1;
    constexpr size_t visible = 6;
    if (selectedAchievement_ < achievementScroll_) achievementScroll_ = selectedAchievement_;
    if (selectedAchievement_ >= achievementScroll_ + visible)
        achievementScroll_ = selectedAchievement_ - visible + 1;
    const size_t maxStart = records.size() > visible ? records.size() - visible : 0;
    achievementScroll_ = std::min(achievementScroll_, maxStart);
}

void App::OpenAchievements(const std::string& packageId, Page returnPage, bool fromOverlay) {
    achievementPackageId_ = packageId;
    achievementsReturnPage_ = returnPage;
    achievementsFromOverlay_ = fromOverlay;
    selectedAchievement_ = 0;
    achievementScroll_ = 0;
    ClampAchievementSelection();
    NotifyFocusMoved();
    if (!fromOverlay) NavigateTo(Page::Achievements);
}

void App::HandleAchievementInput(const InputSnapshot& in) {
    const auto records = achievementPackageId_.empty()
        ? std::vector<AchievementRecord>{}
        : runtime_.Achievements(achievementPackageId_);

    if (in.back || in.menu) {
        if (achievementsFromOverlay_) {
            achievementsFromOverlay_ = false;
            NotifyFocusMoved();
        } else {
            const auto returnPage = achievementsReturnPage_;
            NavigateTo(returnPage);
        }
        return;
    }

    if (records.empty()) return;
    if (in.up && selectedAchievement_ > 0) {
        --selectedAchievement_;
        ClampAchievementSelection();
        NotifyFocusMoved();
    }
    if (in.down && selectedAchievement_ + 1 < records.size()) {
        ++selectedAchievement_;
        ClampAchievementSelection();
        NotifyFocusMoved();
    }
}

void App::DrawAchievements(float width, float height) {
    const auto records = achievementPackageId_.empty()
        ? std::vector<AchievementRecord>{}
        : runtime_.Achievements(achievementPackageId_);

    DrawTextLine(L"Achievements", 62, 154, 500, 60, true);
    DrawTextLine(Widen(achievementPackageId_), 64, 204, width - 128, 30, false, brushMuted_.Get());

    if (records.empty()) {
        DrawEmptyState(L"No achievements unlocked",
            L"ZERO only shows achievements actually reported and persisted by this game. Locked or unreported achievements are not fabricated.", width);
        DrawTextLine(L"B Back", 64, height - 78, 180, 30, false, brushMuted_.Get());
        return;
    }

    const size_t end = std::min(records.size(), achievementScroll_ + 6);
    float y = 270.0f;
    for (size_t i = achievementScroll_; i < end; ++i, y += 78.0f) {
        const auto& record = records[i];
        const auto rect = D2D1::RectF(62, y, width - 62, y + 62);
        if (i == selectedAchievement_) {
            DrawRoundedCard(rect, 18, brushAccent_.Get());
            ComPtr<ID2D1SolidColorBrush> white;
            target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
            DrawTextLine(record.title.empty() ? Widen(record.id) : Widen(record.title), 92, y + 8, width - 470, 28, false, white.Get());
            DrawTextLine(Widen(record.id), 92, y + 34, width - 470, 22, false, white.Get());
            DrawTextLine(Widen(record.unlockedAtUtc), width - 360, y + 20, 270, 26, false, white.Get());
            DrawFocusRing(rect, 18, true);
        } else {
            DrawRoundedCard(rect, 18, brushCard_.Get());
            DrawTextLine(record.title.empty() ? Widen(record.id) : Widen(record.title), 92, y + 8, width - 470, 28, false);
            DrawTextLine(Widen(record.id), 92, y + 34, width - 470, 22, false, brushMuted_.Get());
            DrawTextLine(Widen(record.unlockedAtUtc), width - 360, y + 20, 270, 26, false, brushMuted_.Get());
        }
    }

    DrawTextLine(std::to_wstring(records.size()) + L" unlocked   ·   Up/Down Browse   ·   B Back",
        64, height - 78, width - 128, 30, false, brushMuted_.Get());
}

} // namespace zero
