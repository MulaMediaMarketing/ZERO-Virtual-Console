#include "App.h"
#include "features/devices/DevicesFeature.h"
#include "features/downloads/DownloadsFeature.h"
#include "features/notifications/NotificationsFeature.h"
#include "features/profile/ProfileFeature.h"
#include <algorithm>

namespace zero {

void App::DrawProfile(float width, float height) {
    const auto model = features::profile::ProfileController::Build(identity_, registry_, runtime_, settings_);
    const auto& profile = model.identity;

    DrawTextLine(L"Profile", 62, 132, 500, 60, true);
    DrawTextLine(profile.localOnly ? L"LOCAL ZERO ID" : L"ZERO ID ONLINE", 64, 190, 320, 28, false, brushMuted_.Get());
    DrawRoundedCard(D2D1::RectF(62, 245, width - 62, 390), 24, brushCard_.Get());
    DrawTextLine(profile.displayName.empty() ? L"Player" : Widen(profile.displayName), 94, 274, width - 380, 44, true);
    DrawTextLine(profile.zeroId.empty() ? L"Local identity unavailable" : Widen(profile.zeroId), 94, 326, width - 380, 28, false, brushMuted_.Get());
    DrawTextLine(profile.localOnly ? L"Local-first identity" : L"Connected identity", width - 300, 276, 210, 28, false, brushMuted_.Get());
    DrawTextLine(Widen(features::profile::ProfileView::PrivacySummary(model)), width - 300, 318, 210, 24, false, brushMuted_.Get());

    const float gap = 14.0f;
    const float cardWidth = (width - 124.0f - gap * 3.0f) / 4.0f;
    const wchar_t* labels[] = {L"GAMES", L"ACHIEVEMENTS", L"PLAYTIME", L"LAUNCHES"};
    const std::wstring values[] = {
        std::to_wstring(model.gameCount),
        std::to_wstring(model.achievementCount),
        std::to_wstring(model.playtimeSeconds / 3600) + L"h",
        std::to_wstring(model.launchCount)
    };
    for (int i = 0; i < 4; ++i) {
        const float x = 62.0f + i * (cardWidth + gap);
        DrawRoundedCard(D2D1::RectF(x, 425, x + cardWidth, 530), 20, brushCard_.Get());
        DrawTextLine(values[i], x + 22, 448, cardWidth - 44, 38, true);
        DrawTextLine(labels[i], x + 22, 491, cardWidth - 44, 22, false, brushMuted_.Get());
    }

    DrawTextLine(L"RECENT ACTIVITY", 64, 575, 300, 28, false, brushMuted_.Get());
    if (model.recent.empty()) {
        DrawTextLine(L"No local game activity yet.", 64, 620, width - 128, 30, false, brushMuted_.Get());
    } else {
        float y = 620.0f;
        for (const auto& activity : model.recent) {
            DrawRoundedCard(D2D1::RectF(62, y, width - 62, y + 46), 14, brushCard_.Get());
            DrawTextLine(Widen(activity.title), 84, y + 9, width - 520, 26, false);
            DrawTextLine(Widen(activity.lastPlayedAtUtc.empty() ? "Never played" : activity.lastPlayedAtUtc), width - 390, y + 10, 300, 24, false, brushMuted_.Get());
            y += 58.0f;
        }
    }

    DrawTextLine(L"Privacy defaults to private. Online-only fields stay hidden until ZERO identity is authoritative.", 64, height - 70, width - 128, 28, false, brushMuted_.Get());
}

void App::DrawDownloads(float width, float height) {
    const auto model = features::downloads::DownloadsController::Build(downloads_, registry_);

    DrawTextLine(L"Downloads", 62, 132, 500, 60, true);
    DrawTextLine(L"INSTALLS, UPDATES AND TRANSFERS", 64, 190, 520, 28, false, brushMuted_.Get());

    if (model.jobs.empty()) {
        DrawRoundedCard(D2D1::RectF(62, 245, width - 62, 360), 22, brushCard_.Get());
        DrawTextLine(L"No active transfers", 90, 274, 420, 38, true);
        DrawTextLine(L"ZERO will only show real queued, downloading, paused, verifying, installing, ready or failed jobs here.", 90, 318, width - 180, 30, false, brushMuted_.Get());
    } else {
        DrawTextLine(std::to_wstring(model.jobs.size()) + L" ACTIVE / RECENT JOBS", 64, 245, 360, 28, false, brushMuted_.Get());
        float y = 285.0f;
        for (size_t i = 0; i < std::min<size_t>(5, model.jobs.size()); ++i, y += 72.0f) {
            const auto& job = model.jobs[i];
            const auto rect = D2D1::RectF(62, y, width - 62, y + 58);
            DrawRoundedCard(rect, 17, brushCard_.Get());
            DrawTextLine(Widen(job.packageId), 88, y + 6, width - 600, 25, false);
            DrawTextLine(Widen(job.stateLabel) + L"  ·  " + std::to_wstring(job.percent) + L"%", width - 430, y + 8, 340, 24, false, brushMuted_.Get());
            if (!job.failure.empty()) DrawTextLine(Widen(job.failure), 88, y + 31, width - 176, 20, false, brushMuted_.Get());
        }
    }

    const float installsY = model.jobs.empty() ? 405.0f : 665.0f;
    DrawTextLine(L"LOCAL INSTALLS", 64, installsY, 300, 28, false, brushMuted_.Get());
    if (model.installed.empty()) {
        DrawTextLine(L"No installed packages.", 64, installsY + 45, width - 128, 30, false, brushMuted_.Get());
    } else if (model.jobs.empty()) {
        float y = installsY + 45.0f;
        for (size_t i = 0; i < std::min<size_t>(5, model.installed.size()); ++i, y += 64.0f) {
            const auto& game = model.installed[i];
            DrawRoundedCard(D2D1::RectF(62, y, width - 62, y + 50), 14, brushCard_.Get());
            DrawTextLine(Widen(game.title), 86, y + 9, width - 460, 26, false);
            DrawTextLine(L"Installed  ·  " + Widen(game.version), width - 360, y + 10, 270, 24, false, brushMuted_.Get());
        }
    }
    DrawTextLine(L"The V5 DownloadAuthority owns queue state. Jobs appear only after an authoritative source creates them.",
                 64, height - 70, width - 128, 28, false, brushMuted_.Get());
}

void App::DrawDevices(float width, float height) {
    const auto model = features::devices::DevicesController::Build(input_);

    DrawTextLine(L"Devices", 62, 132, 500, 60, true);
    DrawTextLine(L"LOCAL HARDWARE + VERIFIED ZERO DEVICES", 64, 190, 620, 28, false, brushMuted_.Get());

    DrawRoundedCard(D2D1::RectF(62, 245, width - 62, 350), 22, brushCard_.Get());
    DrawTextLine(L"This PC", 92, 270, 360, 36, true);
    DrawTextLine(L"Windows 11 x64 local player", 92, 312, 420, 26, false, brushMuted_.Get());
    DrawTextLine(L"LOCAL", width - 220, 286, 130, 26, false, brushMuted_.Get());

    DrawRoundedCard(D2D1::RectF(62, 375, width - 62, 480), 22, brushCard_.Get());
    DrawTextLine(L"Controller", 92, 400, 360, 36, true);
    if (model.controllerConnected) {
        DrawTextLine(L"XInput controller connected  ·  slot " + std::to_wstring(model.controllerSlot + 1), 92, 442, 520, 26, false, brushMuted_.Get());
        DrawTextLine(Widen(features::devices::DevicesView::ControllerStatus(model)), width - 220, 416, 130, 26, false, brushMuted_.Get());
    } else {
        DrawTextLine(L"No controller currently connected", 92, 442, 520, 26, false, brushMuted_.Get());
        DrawTextLine(Widen(features::devices::DevicesView::ControllerStatus(model)), width - 220, 416, 130, 26, false, brushMuted_.Get());
    }

    DrawTextLine(L"TRUSTED ZERO DEVICES", 64, 530, 360, 28, false, brushMuted_.Get());
    DrawTextLine(L"No server-authoritative paired devices are currently available. ZERO does not synthesize device records.",
                 64, 575, width - 128, 54, false, brushMuted_.Get());
    DrawTextLine(L"Controller state refreshes live while this page is open.", 64, height - 70, width - 128, 28, false, brushMuted_.Get());
}

void App::DrawNotifications(float width, float height) {
    const auto model = features::notifications::NotificationsController::Build(status_, lastRuntimeOutcome_, captures_);

    DrawTextLine(L"Notifications", 62, 132, 500, 60, true);
    DrawTextLine(L"SYSTEM + GAME ACTIVITY", 64, 190, 420, 28, false, brushMuted_.Get());

    float y = 250.0f;
    for (const auto& item : model.items) {
        DrawRoundedCard(D2D1::RectF(62, y, width - 62, y + 72), 18, brushCard_.Get());
        DrawTextLine(Widen(item.title), 88, y + 9, 420, 26, false);
        DrawTextLine(Widen(item.body), 88, y + 37, width - 176, 25, false, brushMuted_.Get());
        y += 86.0f;
    }

    if (features::notifications::NotificationsView::IsEmpty(model)) {
        DrawEmptyState(L"You're all caught up",
            L"ZERO has no local system or game events requiring attention. Online social, commerce and cloud notifications appear only from authoritative services.", width);
    }
    DrawTextLine(L"No synthetic alerts are created.", 64, height - 70, width - 128, 28, false, brushMuted_.Get());
}

void App::DrawDiscover(float width, float height) {
    DrawTextLine(L"Discover", 62, 132, 500, 60, true);
    DrawTextLine(L"DISCOVER YOUR ZERO LIBRARY", 64, 190, 440, 28, false, brushMuted_.Get());

    if (registry_.Games().empty()) {
        DrawEmptyState(L"Nothing to discover yet",
            L"Import your first validated game from Library. Online catalog recommendations remain hidden until authoritative ZERO catalog data is connected.", width);
        return;
    }

    std::vector<size_t> order(registry_.Games().size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [this](size_t a, size_t b) {
        const auto& games = registry_.Games();
        const auto sa = runtime_.PlatformState(games[a].packageId);
        const auto sb = runtime_.PlatformState(games[b].packageId);
        if (sa.lastPlayedAtUtc != sb.lastPlayedAtUtc) return sa.lastPlayedAtUtc > sb.lastPlayedAtUtc;
        return games[a].title < games[b].title;
    });

    DrawTextLine(L"FROM YOUR LIBRARY", 64, 240, 300, 28, false, brushMuted_.Get());
    const float gap = 16.0f;
    const float cardWidth = (width - 124.0f - gap * 2.0f) / 3.0f;
    for (size_t i = 0; i < std::min<size_t>(6, order.size()); ++i) {
        const size_t row = i / 3;
        const size_t col = i % 3;
        const float x = 62.0f + col * (cardWidth + gap);
        const float y = 285.0f + row * 150.0f;
        const auto& game = registry_.Games()[order[i]];
        const auto state = runtime_.PlatformState(game.packageId);
        DrawRoundedCard(D2D1::RectF(x, y, x + cardWidth, y + 126), 20, brushCard_.Get());
        DrawTextLine(Widen(game.title), x + 22, y + 20, cardWidth - 44, 32, true);
        DrawTextLine(L"Version " + Widen(game.version), x + 22, y + 62, cardWidth - 44, 24, false, brushMuted_.Get());
        DrawTextLine(state.lastPlayedAtUtc.empty() ? L"Not played yet" : L"Recently played", x + 22, y + 91, cardWidth - 44, 22, false, brushMuted_.Get());
    }

    DrawTextLine(L"Online recommendations, store trends and personalized catalog rows are never fabricated while catalog services are disconnected.",
                 64, height - 70, width - 128, 28, false, brushMuted_.Get());
}

} // namespace zero
