#include "App.h"
#include <algorithm>

using Microsoft::WRL::ComPtr;

namespace zero {

namespace {

const v5::AchievementUnlock* findAuthoritativeUnlock(const std::vector<v5::AchievementUnlock>& unlocks,
                                                      const std::string& achievementId) {
    const auto it = std::find_if(unlocks.begin(), unlocks.end(), [&](const v5::AchievementUnlock& unlock) {
        return unlock.achievementId == achievementId;
    });
    return it == unlocks.end() ? nullptr : &*it;
}

} // namespace

void App::ClampAchievementSelection() {
    if (achievementPackageId_.empty()) {
        selectedAchievement_ = 0;
        achievementScroll_ = 0;
        return;
    }

    const auto definitions = runtime_.AchievementDefinitions(achievementPackageId_);
    const auto records = runtime_.Achievements(achievementPackageId_);
    const size_t count = definitions.empty() ? records.size() : definitions.size();
    if (count == 0) {
        selectedAchievement_ = 0;
        achievementScroll_ = 0;
        return;
    }
    if (selectedAchievement_ >= count) selectedAchievement_ = count - 1;
    constexpr size_t visible = 4;
    if (selectedAchievement_ < achievementScroll_) achievementScroll_ = selectedAchievement_;
    if (selectedAchievement_ >= achievementScroll_ + visible)
        achievementScroll_ = selectedAchievement_ - visible + 1;
    const size_t maxStart = count > visible ? count - visible : 0;
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

    if (achievementPackageId_.empty()) return;
    const auto definitions = runtime_.AchievementDefinitions(achievementPackageId_);
    const auto records = runtime_.Achievements(achievementPackageId_);
    const size_t count = definitions.empty() ? records.size() : definitions.size();
    if (count == 0) return;

    if (in.up && selectedAchievement_ > 0) {
        --selectedAchievement_;
        ClampAchievementSelection();
        NotifyFocusMoved();
    }
    if (in.down && selectedAchievement_ + 1 < count) {
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
        std::size_t gamesWithAchievements = 0;

        for (const auto& game : registry_.Games()) {
            const auto state = runtime_.PlatformState(game.packageId);
            totalPlaytimeSeconds += state.totalPlaytimeSeconds;

            const auto records = runtime_.Achievements(game.packageId);
            if (!records.empty()) ++gamesWithAchievements;
            for (const auto& record : records) recent.push_back({record, game.title});
        }

        std::stable_sort(recent.begin(), recent.end(), [](const GlobalAchievementRow& a, const GlobalAchievementRow& b) {
            return a.record.unlockedAtUtc > b.record.unlockedAtUtc;
        });

        const auto identity = identity_.CurrentProfile();
        const auto authoritativeScore = identity.localOnly ? 0ull : runtime_.AuthoritativeAchievementScore(identity.zeroId);

        DrawTextLine(L"Achievements", 62, 132, 500, 60, true);
        DrawTextLine(L"YOUR VERIFIED ACHIEVEMENT HISTORY", 64, 190, width - 128, 30, false, brushMuted_.Get());

        const float gap = 16.0f;
        const float cardWidth = (width - 124.0f - gap * 3.0f) / 4.0f;
        const std::wstring values[] = {
            std::to_wstring(recent.size()),
            std::to_wstring(gamesWithAchievements),
            std::to_wstring(totalPlaytimeSeconds / 3600) + L"h",
            identity.localOnly ? L"Offline" : std::to_wstring(authoritativeScore)
        };
        const wchar_t* labels[] = {L"LOCAL UNLOCKS", L"GAMES", L"PLAYTIME", L"ZERO SCORE"};
        for (int i = 0; i < 4; ++i) {
            const float x = 62.0f + i * (cardWidth + gap);
            DrawRoundedCard(D2D1::RectF(x, 245, x + cardWidth, 350), 22, brushCard_.Get());
            DrawTextLine(values[i], x + 22, 266, cardWidth - 44, 44, true);
            DrawTextLine(labels[i], x + 22, 313, cardWidth - 44, 24, false, brushMuted_.Get());
        }

        DrawTextLine(L"RECENT UNLOCKS", 64, 390, 320, 28, false, brushMuted_.Get());
        if (recent.empty()) {
            DrawEmptyState(L"No achievements unlocked",
                L"ZERO will show real persisted unlocks here after a compatible game reports them. Nothing is fabricated.", width);
        } else {
            const size_t count = std::min<std::size_t>(4, recent.size());
            float y = 430.0f;
            for (size_t i = 0; i < count; ++i, y += 62.0f) {
                const auto& row = recent[i];
                const auto rect = D2D1::RectF(62, y, width - 62, y + 50);
                DrawRoundedCard(rect, 15, brushCard_.Get());
                DrawTextLine(row.record.title.empty() ? Widen(row.record.id) : Widen(row.record.title), 88, y + 5, width - 560, 24, false);
                DrawTextLine(Widen(row.gameTitle), 88, y + 27, width - 560, 20, false, brushMuted_.Get());
                DrawTextLine(Widen(row.record.unlockedAtUtc), width - 390, y + 12, 300, 22, false, brushMuted_.Get());
            }
        }

        DrawTextLine(identity.localOnly
                         ? L"ZERO Score and authoritative global completion require a connected ZERO identity."
                         : L"ZERO Score is derived only from authoritative ZERO-service achievement definitions and unlocks.",
                     64, height - 46, width - 128, 26, false, brushMuted_.Get());
        return;
    }

    const auto records = runtime_.Achievements(achievementPackageId_);
    const auto definitions = runtime_.AchievementDefinitions(achievementPackageId_);
    const auto identity = identity_.CurrentProfile();
    const auto authorityUnlocks = identity.localOnly
        ? std::vector<v5::AchievementUnlock>{}
        : runtime_.AuthoritativeAchievementUnlocks(identity.zeroId, achievementPackageId_);

    DrawTextLine(L"Achievements", 62, 132, 500, 60, true);
    DrawTextLine(Widen(achievementPackageId_), 64, 190, width - 128, 30, false, brushMuted_.Get());

    if (!definitions.empty()) {
        std::uint64_t totalScore = 0;
        std::uint64_t earnedScore = 0;
        std::size_t unlocked = 0;
        for (const auto& definition : definitions) {
            totalScore += definition.score;
            if (findAuthoritativeUnlock(authorityUnlocks, definition.achievementId)) {
                ++unlocked;
                earnedScore += definition.score;
            }
        }
        const auto completion = static_cast<unsigned>((unlocked * 100u) / definitions.size());

        const float gap = 14.0f;
        const float cardWidth = (width - 124.0f - gap * 2.0f) / 3.0f;
        const std::wstring values[] = {
            identity.localOnly ? L"Offline" : std::to_wstring(completion) + L"%",
            identity.localOnly ? L"Offline" : std::to_wstring(earnedScore) + L" / " + std::to_wstring(totalScore),
            std::to_wstring(definitions.size())
        };
        const wchar_t* labels[] = {L"COMPLETION", L"ZERO SCORE", L"TOTAL"};
        for (int i = 0; i < 3; ++i) {
            const float x = 62.0f + i * (cardWidth + gap);
            DrawRoundedCard(D2D1::RectF(x, 238, x + cardWidth, 325), 20, brushCard_.Get());
            DrawTextLine(values[i], x + 22, 252, cardWidth - 44, 34, true);
            DrawTextLine(labels[i], x + 22, 292, cardWidth - 44, 22, false, brushMuted_.Get());
        }

        const size_t end = std::min(definitions.size(), achievementScroll_ + 4);
        float y = 350.0f;
        for (size_t i = achievementScroll_; i < end; ++i, y += 68.0f) {
            const auto& definition = definitions[i];
            const auto* unlock = findAuthoritativeUnlock(authorityUnlocks, definition.achievementId);
            const bool isUnlocked = unlock != nullptr;
            const bool hideSecret = definition.secret && !isUnlocked;
            const auto rect = D2D1::RectF(62, y, width - 62, y + 54);
            DrawRoundedCard(rect, 16, i == selectedAchievement_ ? brushAccent_.Get() : brushCard_.Get());
            if (i == selectedAchievement_) DrawFocusRing(rect, 16, true);

            const std::wstring title = hideSecret ? L"Secret achievement" : Widen(definition.title);
            const std::wstring description = hideSecret ? L"Details hidden until unlocked" : Widen(definition.description);
            DrawTextLine(title, 92, y + 4, width - 610, 24, false);
            DrawTextLine(description, 92, y + 28, width - 610, 19, false, brushMuted_.Get());

            std::wstring status;
            if (hideSecret) {
                status = L"SECRET  ·  LOCKED";
            } else {
                status = isUnlocked ? L"UNLOCKED" : L"LOCKED";
                status += L"  ·  " + std::to_wstring(definition.score) + L" pts";
                if (definition.targetProgress > 1 && !identity.localOnly) {
                    const auto progress = runtime_.AuthoritativeAchievementProgress(
                        identity.zeroId, achievementPackageId_, definition.achievementId);
                    const auto current = progress ? progress->currentProgress : 0u;
                    status += L"  ·  " + std::to_wstring(current) + L"/" + std::to_wstring(definition.targetProgress);
                }
                if (unlock) {
                    const double rarity = static_cast<double>(unlock->rarityBasisPoints) / 100.0;
                    status += L"  ·  " + std::to_wstring(rarity).substr(0, 4) + L"% rarity";
                }
            }
            DrawTextLine(status, width - 500, y + 15, 410, 22, false, brushMuted_.Get());
        }

        DrawTextLine(identity.localOnly
                         ? L"Authoritative definitions are synced, but account completion/progress stays hidden until ZERO identity is connected."
                         : L"Up/Down Browse · B Back · Progress, rarity and secret details come only from authoritative ZERO data.",
                     64, height - 46, width - 128, 26, false, brushMuted_.Get());
        return;
    }

    DrawRoundedCard(D2D1::RectF(62, 238, width - 62, 325), 20, brushCard_.Get());
    DrawTextLine(std::to_wstring(records.size()), 88, 253, 160, 40, true);
    DrawTextLine(L"LOCAL UNLOCKS", 88, 292, 200, 22, false, brushMuted_.Get());
    DrawTextLine(L"Authoritative definitions are not currently available, so ZERO will not invent locked entries, rarity, score, progress or completion.",
                 300, 258, width - 390, 52, false, brushMuted_.Get());

    if (records.empty()) {
        DrawEmptyState(L"No achievements unlocked",
            L"ZERO only shows achievements actually reported and persisted by this game. Locked or unreported achievements are not fabricated.", width);
        DrawTextLine(L"B Back", 64, height - 46, 180, 26, false, brushMuted_.Get());
        return;
    }

    const size_t end = std::min(records.size(), achievementScroll_ + 4);
    float y = 350.0f;
    for (size_t i = achievementScroll_; i < end; ++i, y += 68.0f) {
        const auto& record = records[i];
        const auto rect = D2D1::RectF(62, y, width - 62, y + 54);
        if (i == selectedAchievement_) {
            DrawRoundedCard(rect, 16, brushAccent_.Get());
            ComPtr<ID2D1SolidColorBrush> white;
            target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
            DrawTextLine(record.title.empty() ? Widen(record.id) : Widen(record.title), 92, y + 5, width - 470, 24, false, white.Get());
            DrawTextLine(Widen(record.id), 92, y + 29, width - 470, 19, false, white.Get());
            DrawTextLine(Widen(record.unlockedAtUtc), width - 360, y + 14, 270, 22, false, white.Get());
            DrawFocusRing(rect, 16, true);
        } else {
            DrawRoundedCard(rect, 16, brushCard_.Get());
            DrawTextLine(record.title.empty() ? Widen(record.id) : Widen(record.title), 92, y + 5, width - 470, 24, false);
            DrawTextLine(Widen(record.id), 92, y + 29, width - 470, 19, false, brushMuted_.Get());
            DrawTextLine(Widen(record.unlockedAtUtc), width - 360, y + 14, 270, 22, false, brushMuted_.Get());
        }
    }

    DrawTextLine(std::to_wstring(records.size()) + L" local unlocks · Up/Down Browse · B Back",
        64, height - 46, width - 128, 26, false, brushMuted_.Get());
}

} // namespace zero
