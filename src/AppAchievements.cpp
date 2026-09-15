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
    (void)returnPage;
    achievementPackageId_ = packageId;
    achievementsFromOverlay_ = fromOverlay;
    selectedAchievement_ = 0;
    achievementScroll_ = 0;
    ClampAchievementSelection();
    NotifyFocusMoved();

    if (!fromOverlay) {
        const auto beforeRevision = productionShell_.Snapshot().navigationRevision;
        std::string error;
        if (!productionShell_.NavigateContextual(Page::Achievements, error)) {
            status_ = Widen(error.empty() ? "ZERO could not open Achievements." : error);
            return;
        }
        if (productionShell_.Snapshot().navigationRevision != beforeRevision) {
            shellUx_.BeginPageTransition();
            NotifyFocusMoved();
        }
        status_.clear();
    }
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
            const auto beforeRevision = productionShell_.Snapshot().navigationRevision;
            std::string error;
            if (!productionShell_.Back(error)) {
                status_ = Widen(error.empty() ? "ZERO could not return from Achievements." : error);
                return;
            }
            if (productionShell_.Snapshot().navigationRevision != beforeRevision) {
                shellUx_.BeginPageTransition();
                NotifyFocusMoved();
            }
            status_.clear();
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
    if (achievementPackageId_.empty()) {
        struct GlobalAchievementRow {
            AchievementRecord record;
            std::string gameTitle;
        };

        std::vector<GlobalAchievementRow> recent;
        std::uint64_t totalPlaytimeSeconds = 0;
        std::uint64_t totalLaunches = 0;
        std::size_t gamesWithAchievements = 0;

        for (const auto& game : registry_.Games()) {
            const auto state = runtime_.PlatformState(game.packageId);
            totalPlaytimeSeconds += state.totalPlaytimeSeconds;
            totalLaunches += state.launchCount;

            const auto records = runtime_.Achievements(game.packageId);
            if (!records.empty()) ++gamesWithAchievements;
            for (const auto& record : records) recent.push_back({record, game.title});
        }

        std::stable_sort(recent.begin(), recent.end(), [](const GlobalAchievementRow& a, const GlobalAchievementRow& b) {
            return a.record.unlockedAtUtc > b.record.unlockedAtUtc;
        });

        DrawTextLine(L"Achievements", 62, 132, 500, 60, true);
        DrawTextLine(L"YOUR VERIFIED LOCAL ACHIEVEMENT HISTORY", 64, 190, width - 128, 30, false, brushMuted_.Get());

        const float gap = 16.0f;
        const float cardWidth = (width - 124.0f - gap * 2.0f) / 3.0f;
        const auto unlockedCard = D2D1::RectF(62, 245, 62 + cardWidth, 350);
        const auto gamesCard = D2D1::RectF(62 + cardWidth + gap, 245, 62 + cardWidth * 2.0f + gap, 350);
        const auto timeCard = D2D1::RectF(62 + cardWidth * 2.0f + gap * 2.0f, 245, width - 62, 350);
        DrawRoundedCard(unlockedCard, 22, brushCard_.Get());
        DrawRoundedCard(gamesCard, 22, brushCard_.Get());
        DrawRoundedCard(timeCard, 22, brushCard_.Get());
        DrawTextLine(std::to_wstring(recent.size()), unlockedCard.left + 24, 266, cardWidth - 48, 44, true);
        DrawTextLine(L"UNLOCKED", unlockedCard.left + 24, 313, cardWidth - 48, 24, false, brushMuted_.Get());
        DrawTextLine(std::to_wstring(gamesWithAchievements), gamesCard.left + 24, 266, cardWidth - 48, 44, true);
        DrawTextLine(L"GAMES WITH UNLOCKS", gamesCard.left + 24, 313, cardWidth - 48, 24, false, brushMuted_.Get());
        DrawTextLine(std::to_wstring(totalPlaytimeSeconds / 3600) + L"h", timeCard.left + 24, 266, cardWidth - 48, 44, true);
        DrawTextLine(L"VERIFIED PLAYTIME", timeCard.left + 24, 313, cardWidth - 48, 24, false, brushMuted_.Get());

        DrawTextLine(L"RECENT UNLOCKS", 64, 390, 320, 28, false, brushMuted_.Get());
        if (recent.empty()) {
            DrawEmptyState(L"No achievements unlocked",
                L"ZERO will show real persisted unlocks here after a compatible game reports them. Nothing is fabricated.", width);
        } else {
            const size_t count = std::min<std::size_t>(6, recent.size());
            float y = 430.0f;
            for (size_t i = 0; i < count; ++i, y += 68.0f) {
                const auto& row = recent[i];
                const auto rect = D2D1::RectF(62, y, width - 62, y + 54);
                DrawRoundedCard(rect, 16, brushCard_.Get());
                DrawTextLine(row.record.title.empty() ? Widen(row.record.id) : Widen(row.record.title), 88, y + 6, width - 560, 26, false);
                DrawTextLine(Widen(row.gameTitle), 88, y + 29, width - 560, 21, false, brushMuted_.Get());
                DrawTextLine(Widen(row.record.unlockedAtUtc), width - 390, y + 14, 300, 24, false, brushMuted_.Get());
            }
        }

        DrawTextLine(L"ZERO only displays persisted local unlocks until authoritative achievement definitions are synced.",
                     64, height - 70, width - 128, 28, false, brushMuted_.Get());
        (void)totalLaunches;
        return;
    }

    const auto records = runtime_.Achievements(achievementPackageId_);

    DrawTextLine(L"Achievements", 62, 132, 500, 60, true);
    DrawTextLine(Widen(achievementPackageId_), 64, 190, width - 128, 30, false, brushMuted_.Get());

    DrawRoundedCard(D2D1::RectF(62, 238, width - 62, 325), 20, brushCard_.Get());
    DrawTextLine(std::to_wstring(records.size()), 88, 253, 160, 40, true);
    DrawTextLine(L"UNLOCKED", 88, 292, 180, 22, false, brushMuted_.Get());
    DrawTextLine(L"Completion %, rarity, ZERO Score, progress and locked/secret entries appear only when authoritative definitions are available.",
                 270, 258, width - 360, 52, false, brushMuted_.Get());

    if (records.empty()) {
        DrawEmptyState(L"No achievements unlocked",
            L"ZERO only shows achievements actually reported and persisted by this game. Locked or unreported achievements are not fabricated.", width);
        DrawTextLine(L"B Back", 64, height - 78, 180, 30, false, brushMuted_.Get());
        return;
    }

    const size_t end = std::min(records.size(), achievementScroll_ + 6);
    float y = 350.0f;
    for (size_t i = achievementScroll_; i < end; ++i, y += 72.0f) {
        const auto& record = records[i];
        const auto rect = D2D1::RectF(62, y, width - 62, y + 58);
        if (i == selectedAchievement_) {
            DrawRoundedCard(rect, 17, brushAccent_.Get());
            ComPtr<ID2D1SolidColorBrush> white;
            target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
            DrawTextLine(record.title.empty() ? Widen(record.id) : Widen(record.title), 92, y + 6, width - 470, 26, false, white.Get());
            DrawTextLine(Widen(record.id), 92, y + 31, width - 470, 20, false, white.Get());
            DrawTextLine(Widen(record.unlockedAtUtc), width - 360, y + 16, 270, 24, false, white.Get());
            DrawFocusRing(rect, 17, true);
        } else {
            DrawRoundedCard(rect, 17, brushCard_.Get());
            DrawTextLine(record.title.empty() ? Widen(record.id) : Widen(record.title), 92, y + 6, width - 470, 26, false);
            DrawTextLine(Widen(record.id), 92, y + 31, width - 470, 20, false, brushMuted_.Get());
            DrawTextLine(Widen(record.unlockedAtUtc), width - 360, y + 16, 270, 24, false, brushMuted_.Get());
        }
    }

    DrawTextLine(std::to_wstring(records.size()) + L" unlocked   ·   Up/Down Browse   ·   B Back",
        64, height - 78, width - 128, 30, false, brushMuted_.Get());
}

} // namespace zero
