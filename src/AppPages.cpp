#include "App.h"
#include <algorithm>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace zero {
namespace {
std::wstring captureKindLabel(CaptureKind kind) {
    return kind == CaptureKind::Screenshot ? L"Screenshot" : L"Video clip";
}

std::wstring fileSizeLabel(uint64_t bytes) {
    if (bytes >= 1024ull * 1024ull * 1024ull)
        return std::to_wstring(bytes / (1024ull * 1024ull * 1024ull)) + L" GB";
    if (bytes >= 1024ull * 1024ull)
        return std::to_wstring(bytes / (1024ull * 1024ull)) + L" MB";
    if (bytes >= 1024ull)
        return std::to_wstring(bytes / 1024ull) + L" KB";
    return std::to_wstring(bytes) + L" B";
}
}

void App::DrawRecentGames(float width, float height) {
    const auto& games = registry_.Games();
    if (games.empty() || height < 780.0f) return;

    std::vector<size_t> order(games.size());
    for (size_t i = 0; i < games.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [this, &games](size_t a, size_t b) {
        return runtime_.PlatformState(games[a].packageId).lastPlayedAtUtc >
               runtime_.PlatformState(games[b].packageId).lastPlayedAtUtc;
    });

    DrawTextLine(L"Recently Played", 64, 650, 260, 30, false, brushMuted_.Get());
    const size_t count = std::min<size_t>(4, order.size());
    const float gap = 14.0f;
    const float cardWidth = (width - 128.0f - gap * 3.0f) / 4.0f;
    for (size_t i = 0; i < count; ++i) {
        const auto& game = games[order[i]];
        const float x = 64.0f + static_cast<float>(i) * (cardWidth + gap);
        const auto rect = D2D1::RectF(x, 690, x + cardWidth, 755);
        DrawRoundedCard(rect, 16, brushCard_.Get());
        DrawTextLine(Widen(game.title), x + 18, 710, cardWidth - 36, 30, false);
    }
}

void App::DrawEmptyState(const std::wstring& title, const std::wstring& body, float width) {
    const auto rect = D2D1::RectF(62, 270, width - 62, 540);
    DrawRoundedCard(rect, 28, brushCard_.Get());
    DrawTextLine(title, 100, 322, width - 200, 52, true);
    DrawTextLine(body, 102, 390, width - 230, 88, false, brushMuted_.Get());
}

void App::DrawStore(float width, float height) {
    (void)height;
    DrawTextLine(L"Store", 62, 154, 500, 60, true);
    DrawTextLine(L"ZERO Store", 64, 204, 300, 30, false, brushMuted_.Get());

    const auto state = store_.State();
    const auto& products = store_.Products();
    if (state == StoreProviderState::Disconnected) {
        DrawEmptyState(L"Store service is not connected",
            L"Catalog, checkout, entitlement, CDN, and publisher services will populate this destination when a real Store provider is connected. ZERO does not inject fake products or prices.",
            width);
        return;
    }
    if (state == StoreProviderState::Error) {
        DrawEmptyState(L"ZERO Store is unavailable",
            L"The connected Store provider reported an error. Your installed library remains available while the service recovers.", width);
        return;
    }
    if (products.empty()) {
        DrawEmptyState(L"No products available",
            L"The Store provider is connected but returned no catalog entries.", width);
        return;
    }

    float y = 280.0f;
    const size_t count = std::min<size_t>(6, products.size());
    for (size_t i = 0; i < count; ++i, y += 80.0f) {
        const auto& product = products[i];
        const auto rect = D2D1::RectF(62, y, width - 62, y + 64);
        DrawRoundedCard(rect, 18, brushCard_.Get());
        DrawTextLine(Widen(product.title), 92, y + 10, 430, 30, false);
        DrawTextLine(Widen(product.shortDescription), 92, y + 34, width - 520, 24, false, brushMuted_.Get());
        std::wstring right = Widen(product.displayPrice);
        if (product.entitlement == EntitlementState::Owned) right = L"Owned";
        DrawTextLine(right, width - 260, y + 18, 170, 30, false, brushMuted_.Get());
    }
}

void App::DrawFriends(float width, float height) {
    (void)height;
    const auto profile = identity_.CurrentProfile();
    const auto state = friends_.State();
    const auto& list = friends_.Friends();

    DrawTextLine(L"Friends", 62, 154, 500, 60, true);
    DrawTextLine(L"Zero Link", 64, 204, 300, 30, false, brushMuted_.Get());

    const auto profileRect = D2D1::RectF(62, 265, width - 62, 370);
    DrawRoundedCard(profileRect, 24, brushCard_.Get());
    DrawTextLine(profile.displayName.empty() ? L"Player" : Widen(profile.displayName), 92, 286, 420, 42, true);
    DrawTextLine(profile.zeroId.empty() ? L"Local identity unavailable" : Widen(profile.zeroId), 92, 330, width - 360, 30, false, brushMuted_.Get());
    DrawTextLine(profile.localOnly ? L"Local Zero ID" : L"Zero ID", width - 260, 298, 170, 30, false, brushMuted_.Get());

    if (state == FriendsProviderState::Disconnected) {
        DrawRoundedCard(D2D1::RectF(62, 395, width - 62, 590), 24, brushCard_.Get());
        DrawTextLine(L"Zero Link is not connected", 92, 430, 520, 48, true);
        DrawTextLine(L"Presence, requests, invites, joinability, and friend activity will appear here when a real social provider is connected. No placeholder users are shown.",
            94, 490, width - 220, 70, false, brushMuted_.Get());
        return;
    }
    if (state == FriendsProviderState::Error) {
        DrawEmptyState(L"Zero Link is unavailable",
            L"The social provider reported an error. ZERO keeps the rest of the console usable without inventing social data.", width);
        return;
    }
    if (list.empty()) {
        DrawEmptyState(L"No friends to show",
            L"Zero Link is connected, but the provider returned no friend or presence records.", width);
        return;
    }

    float y = 405.0f;
    const size_t count = std::min<size_t>(5, list.size());
    for (size_t i = 0; i < count; ++i, y += 66.0f) {
        const auto& friendItem = list[i];
        DrawRoundedCard(D2D1::RectF(62, y, width - 62, y + 52), 16, brushCard_.Get());
        DrawTextLine(Widen(friendItem.displayName), 90, y + 10, 360, 30, false);
        std::wstring presence = L"Offline";
        if (friendItem.presence == PresenceState::Online) presence = L"Online";
        else if (friendItem.presence == PresenceState::InGame)
            presence = friendItem.gamePackageId.empty() ? L"In game" : L"In game · " + Widen(friendItem.gamePackageId);
        if (friendItem.joinable) presence += L" · Joinable";
        DrawTextLine(presence, width - 500, y + 10, 410, 30, false, brushMuted_.Get());
    }
}

void App::DrawCaptures(float width, float height) {
    (void)height;
    const auto& items = captures_.Items();
    DrawTextLine(L"Captures", 62, 154, 500, 60, true);
    DrawTextLine(std::to_wstring(items.size()) + L" local captures   ·   A View/Open   ·   X Delete   ·   Right Reveal",
        64, 204, width - 128, 30, false, brushMuted_.Get());

    if (items.empty()) {
        DrawEmptyState(L"No captures yet",
            L"Screenshots and clips created through ZERO will appear here. No sample captures are injected.", width);
        return;
    }

    const size_t end = std::min(items.size(), captureScroll_ + 6);
    float y = 270.0f;
    for (size_t i = captureScroll_; i < end; ++i, y += 78.0f) {
        const auto& item = items[i];
        const auto rect = D2D1::RectF(62, y, width - 62, y + 62);
        if (i == selectedCapture_) {
            DrawRoundedCard(rect, 18, brushAccent_.Get());
            ComPtr<ID2D1SolidColorBrush> white;
            target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
            DrawTextLine(item.path.filename().wstring(), 92, y + 10, width - 430, 30, false, white.Get());
            DrawTextLine(captureKindLabel(item.kind) + L"  ·  " + fileSizeLabel(item.sizeBytes), width - 385, y + 10, 300, 30, false, white.Get());
            DrawFocusRing(rect, 18, true);
        } else {
            DrawRoundedCard(rect, 18, brushCard_.Get());
            DrawTextLine(item.path.filename().wstring(), 92, y + 10, width - 430, 30, false);
            DrawTextLine(captureKindLabel(item.kind) + L"  ·  " + fileSizeLabel(item.sizeBytes), width - 385, y + 10, 300, 30, false, brushMuted_.Get());
        }
    }
}

void App::DrawCaptureViewer(float width, float height) {
    if (!captureViewerVisible_) return;
    const auto& items = captures_.Items();
    if (items.empty() || selectedCapture_ >= items.size()) return;
    const auto& item = items[selectedCapture_];

    ComPtr<ID2D1SolidColorBrush> veil;
    ComPtr<ID2D1SolidColorBrush> white;
    target_->CreateSolidColorBrush(D2D1::ColorF(0x000000, 0.94f), veil.GetAddressOf());
    target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
    target_->FillRectangle(D2D1::RectF(0, 0, width, height), veil.Get());

    if (cachedCapturePath_ != item.path) {
        cachedCapture_.Reset();
        cachedCapturePath_ = item.path;
        cachedCapture_ = LoadBitmap(item.path);
    }
    if (cachedCapture_) {
        const auto size = cachedCapture_->GetSize();
        const float maxWidth = width - 160.0f;
        const float maxHeight = height - 180.0f;
        const float scale = std::min(maxWidth / size.width, maxHeight / size.height);
        const float drawWidth = size.width * scale;
        const float drawHeight = size.height * scale;
        const float x = (width - drawWidth) * 0.5f;
        const float y = (height - drawHeight) * 0.5f - 10.0f;
        target_->DrawBitmap(cachedCapture_.Get(), D2D1::RectF(x, y, x + drawWidth, y + drawHeight), 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    } else {
        DrawTextLine(L"ZERO could not decode this screenshot.", 80, height * 0.45f, width - 160, 50, true, white.Get());
    }

    DrawTextLine(item.path.filename().wstring(), 64, 34, width - 128, 36, false, white.Get());
    DrawTextLine(L"B Close   ·   X Delete   ·   Right Reveal", 64, height - 58, width - 128, 30, false, white.Get());
}

void App::DrawCaptureDeleteConfirm(float width, float height) {
    if (!captureDeleteConfirm_) return;
    const auto& items = captures_.Items();
    if (items.empty() || selectedCapture_ >= items.size()) return;

    ComPtr<ID2D1SolidColorBrush> veil;
    ComPtr<ID2D1SolidColorBrush> white;
    target_->CreateSolidColorBrush(D2D1::ColorF(0x000000, 0.70f), veil.GetAddressOf());
    target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
    target_->FillRectangle(D2D1::RectF(0, 0, width, height), veil.Get());

    const float left = std::max(120.0f, width * 0.5f - 330.0f);
    const float top = std::max(120.0f, height * 0.5f - 150.0f);
    const auto dialog = D2D1::RectF(left, top, left + 660, top + 300);
    DrawRoundedCard(dialog, 28, brushAccent_.Get());
    DrawTextLine(L"Delete capture?", left + 40, top + 40, 520, 50, true, white.Get());
    DrawTextLine(items[selectedCapture_].path.filename().wstring(), left + 40, top + 105, 580, 34, false, white.Get());
    DrawTextLine(L"This permanently removes the local capture file.", left + 40, top + 150, 580, 42, false, white.Get());

    const auto action = D2D1::RectF(left + 38, top + 214, left + 238, top + 264);
    DrawRoundedCard(action, 16, white.Get());
    DrawTextLine(L"A   Delete", left + 72, top + 227, 130, 26, false, brushText_.Get());
    DrawFocusRing(action, 16, true);
    DrawTextLine(L"B Cancel", left + 280, top + 227, 140, 26, false, white.Get());
}

void App::DrawOverlay(float width, float height) {
    if (!overlayVisible_) return;

    const float opacity = shellUx_.OverlayOpacity();
    const float slide = shellUx_.OverlayOffsetX();
    ComPtr<ID2D1SolidColorBrush> veil;
    ComPtr<ID2D1SolidColorBrush> panel;
    ComPtr<ID2D1SolidColorBrush> white;
    target_->CreateSolidColorBrush(D2D1::ColorF(0x000000, 0.62f * opacity), veil.GetAddressOf());
    target_->CreateSolidColorBrush(D2D1::ColorF(0x181818, 0.98f), panel.GetAddressOf());
    target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
    target_->FillRectangle(D2D1::RectF(0, 0, width, height), veil.Get());

    const float left = width - 470.0f + slide;
    DrawRoundedCard(D2D1::RectF(left, 40, width - 40 + slide, height - 40), 28, panel.Get());
    DrawTextLine(L"ZERO", left + 34, 75, 250, 48, true, white.Get());
    DrawTextLine(L"SYSTEM OVERLAY", left + 36, 119, 280, 28, false, brushMuted_.Get());

    std::wstring session = L"No active session";
    std::wstring playtime = L"0 min";
    std::wstring achievements = L"0 unlocked";
    if (runtime_.IsActive() || runtime_.State() == RuntimeState::Running) {
        session = Widen(runtime_.Info().title);
        playtime = std::to_wstring(runtime_.PlaytimeSeconds() / 60) + L" min";
        achievements = std::to_wstring(runtime_.Achievements(runtime_.Info().packageId).size()) + L" unlocked";
    }
    DrawTextLine(session, left + 36, 170, 350, 34, false, white.Get());
    DrawTextLine(L"Playtime   " + playtime, left + 36, 210, 350, 30, false, brushMuted_.Get());
    DrawTextLine(L"Achievements   " + achievements, left + 36, 246, 350, 30, false, brushMuted_.Get());

    const wchar_t* options[] = {L"Continue Game", L"Friends", L"Captures", L"Achievements", L"Exit Game"};
    for (size_t i = 0; i < 5; ++i) {
        const float y = 305.0f + static_cast<float>(i) * 66.0f;
        const auto rect = D2D1::RectF(left + 26, y, width - 66 + slide, y + 50);
        if (overlayIndex_ == i) {
            ComPtr<ID2D1SolidColorBrush> selected;
            target_->CreateSolidColorBrush(D2D1::ColorF(0x333333), selected.GetAddressOf());
            DrawRoundedCard(rect, 16, selected.Get());
            DrawFocusRing(rect, 16, true);
        }
        DrawTextLine(options[i], left + 48, y + 11, 300, 30, false, white.Get());
    }
    DrawTextLine(L"A Select   B Close", left + 36, height - 92, 320, 30, false, brushMuted_.Get());
}

void App::Paint() {
    CreateDeviceResources();
    if (!target_) return;

    target_->BeginDraw();
    target_->SetTransform(D2D1::Matrix3x2F::Identity());
    target_->Clear(D2D1::ColorF(0xFBFAF7));

    RECT rc{};
    GetClientRect(hwnd_, &rc);
    const float width = static_cast<float>(rc.right);
    const float height = static_cast<float>(rc.bottom);

    DrawTextLine(L"ZERO", 42, 28, 180, 54, true);
    DrawTextLine(L"VIRTUAL CONSOLE", 44, 68, 210, 28, false, brushMuted_.Get());

    const wchar_t* nav[] = {L"Home", L"Library", L"Store", L"Friends", L"Captures", L"Settings"};
    const float navStart = 300.0f;
    const float navStep = 128.0f;
    const bool topLevel = page_ != Page::GameDetail && page_ != Page::Import;
    for (size_t i = 0; i < 6; ++i) {
        const float x = navStart + static_cast<float>(i) * navStep;
        const auto rect = D2D1::RectF(x - 16, 38, x + 102, 82);
        if (navIndex_ == i) {
            DrawRoundedCard(rect, 18, brushCard_.Get());
            if (topLevel) DrawFocusRing(rect, 18);
        }
        DrawTextLine(nav[i], x, 48, 104, 30, false);
    }

    const float pageOffset = shellUx_.PageOffsetY();
    target_->SetTransform(D2D1::Matrix3x2F::Translation(0.0f, pageOffset));

    const auto& games = registry_.Games();
    if (page_ == Page::Home) {
        const auto profile = identity_.CurrentProfile();
        const std::string greetingName = profile.displayName.empty() ? settings_.profileName : profile.displayName;
        DrawTextLine(L"Good evening, " + Widen(greetingName), 62, 154, 700, 60, true);
        DrawTextLine(L"Your games. One calm console experience.", 64, 204, 650, 40, false, brushMuted_.Get());
        if (games.empty()) {
            DrawEmptyState(L"No games installed", L"Open Library, then use X to import a ZERO-compatible game folder.", width);
        } else {
            const auto& game = games[std::min(selectedGame_, games.size() - 1)];
            const auto hero = D2D1::RectF(62, 285, width - 62, 620);
            DrawHeroArtwork(game, hero);
            ComPtr<ID2D1SolidColorBrush> shade;
            ComPtr<ID2D1SolidColorBrush> white;
            target_->CreateSolidColorBrush(D2D1::ColorF(0x000000, 0.50f), shade.GetAddressOf());
            target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
            target_->FillRectangle(hero, shade.Get());
            DrawTextLine(Widen(game.title), 108, 334, 760, 60, true, white.Get());
            DrawTextLine(L"Version " + Widen(game.version), 110, 392, 400, 35, false, white.Get());
            const auto resume = game.zeroResume ? resumeStore_.Load(game.packageId) : std::nullopt;
            const auto playRect = D2D1::RectF(108, 500, 300, 556);
            DrawRoundedCard(playRect, 20, white.Get());
            DrawFocusRing(playRect, 20, false);
            DrawTextLine(resume ? L"Resume" : L"Play", 160, 515, 120, 30, false, brushText_.Get());
            if (resume) DrawTextLine(L"Continue: " + Widen(resume->displayLabel), 330, 515, 500, 30, false, white.Get());
            DrawRecentGames(width, height);
        }
    } else if (page_ == Page::Library) {
        DrawTextLine(L"Library", 62, 154, 500, 60, true);
        DrawTextLine(std::to_wstring(games.size()) + L" installed   ·   A Open   ·   X Import Game", 64, 204, 620, 30, false, brushMuted_.Get());
        if (games.empty()) {
            DrawEmptyState(L"Your library is empty", L"Press X to import a validated ZERO-compatible native game package.", width);
        } else {
            float y = 280.0f;
            const size_t count = std::min<size_t>(8, games.size());
            for (size_t i = 0; i < count; ++i, y += 78.0f) {
                const auto rect = D2D1::RectF(62, y, width - 62, y + 62);
                if (i == selectedGame_) {
                    DrawRoundedCard(rect, 18, brushCard_.Get());
                    DrawFocusRing(rect, 18);
                }
                DrawTextLine(Widen(games[i].title), 92, y + 14, 540, 30, false);
                DrawTextLine(Widen(games[i].version), width - 250, y + 14, 120, 30, false, brushMuted_.Get());
            }
        }
    } else if (page_ == Page::Store) {
        DrawStore(width, height);
    } else if (page_ == Page::Friends) {
        DrawFriends(width, height);
    } else if (page_ == Page::Captures) {
        DrawCaptures(width, height);
    } else if (page_ == Page::GameDetail && !games.empty()) {
        const auto& game = games[std::min(selectedGame_, games.size() - 1)];
        const auto platform = runtime_.PlatformState(game.packageId);
        const auto achievements = runtime_.Achievements(game.packageId);
        const auto resume = game.zeroResume ? resumeStore_.Load(game.packageId) : std::nullopt;
        const float split = std::max(760.0f, width - 470.0f);
        const auto hero = D2D1::RectF(62, 150, split, 535);

        DrawHeroArtwork(game, hero);
        ComPtr<ID2D1SolidColorBrush> shade;
        ComPtr<ID2D1SolidColorBrush> white;
        target_->CreateSolidColorBrush(D2D1::ColorF(0x000000, 0.48f), shade.GetAddressOf());
        target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
        target_->FillRectangle(hero, shade.Get());
        DrawTextLine(Widen(game.title), 104, 205, split - 150, 58, true, white.Get());
        DrawTextLine(Widen(game.packageId), 106, 263, split - 170, 32, false, white.Get());
        DrawTextLine(L"Version " + Widen(game.version), 106, 300, 380, 30, false, white.Get());

        const auto playRect = D2D1::RectF(104, 430, 282, 488);
        DrawRoundedCard(playRect, 20, preferResume_ && resume ? brushCard_.Get() : white.Get());
        DrawTextLine(L"Play", 166, 446, 90, 30, false, brushText_.Get());
        if (!resume || !preferResume_) DrawFocusRing(playRect, 20, false);

        if (resume) {
            const auto resumeRect = D2D1::RectF(300, 430, 500, 488);
            DrawRoundedCard(resumeRect, 20, preferResume_ ? white.Get() : brushCard_.Get());
            DrawTextLine(L"Resume", 354, 446, 110, 30, false, brushText_.Get());
            if (preferResume_) DrawFocusRing(resumeRect, 20, false);
        }

        const float cardLeft = split + 20.0f;
        DrawRoundedCard(D2D1::RectF(cardLeft, 150, width - 62, 535), 26, brushCard_.Get());
        DrawTextLine(L"Game Status", cardLeft + 28, 180, 260, 42, true);
        DrawTextLine(L"Playtime", cardLeft + 28, 245, 160, 30, false, brushMuted_.Get());
        DrawTextLine(std::to_wstring(platform.totalPlaytimeSeconds / 60) + L" min", width - 250, 245, 150, 30, false);
        DrawTextLine(L"Launches", cardLeft + 28, 285, 160, 30, false, brushMuted_.Get());
        DrawTextLine(std::to_wstring(platform.launchCount), width - 250, 285, 150, 30, false);
        DrawTextLine(L"Achievements", cardLeft + 28, 325, 180, 30, false, brushMuted_.Get());
        DrawTextLine(std::to_wstring(achievements.size()) + L" unlocked", width - 250, 325, 150, 30, false);
        DrawTextLine(L"Resume", cardLeft + 28, 365, 160, 30, false, brushMuted_.Get());
        const std::wstring resumeState = !game.zeroResume ? L"Not supported" : (resume ? L"Checkpoint ready" : L"No checkpoint");
        DrawTextLine(resumeState, width - 280, 365, 180, 30, false);
        DrawTextLine(L"Last session", cardLeft + 28, 405, 160, 30, false, brushMuted_.Get());
        const std::wstring lastSession = platform.launchCount == 0 ? L"Never played" :
            (platform.lastSessionCrashed ? L"Ended unexpectedly" : L"Clean exit");
        DrawTextLine(lastSession, width - 280, 405, 180, 30, false);
        DrawTextLine(L"Last played", cardLeft + 28, 445, 160, 30, false, brushMuted_.Get());
        DrawTextLine(platform.lastPlayedAtUtc.empty() ? L"—" : Widen(platform.lastPlayedAtUtc), width - 330, 445, 230, 30, false);
        DrawTextLine(L"Exit code", cardLeft + 28, 485, 160, 30, false, brushMuted_.Get());
        DrawTextLine(std::to_wstring(platform.lastExitCode), width - 250, 485, 150, 30, false);

        if (resume) {
            DrawRoundedCard(D2D1::RectF(62, 565, width - 62, 635), 20, brushCard_.Get());
            DrawTextLine(L"Continue from", 88, 584, 180, 30, false, brushMuted_.Get());
            DrawTextLine(Widen(resume->displayLabel), 270, 584, width - 360, 30, false);
        }
        DrawTextLine(resume ? L"Left / Right  Choose Play or Resume   ·   A  Launch   ·   B  Library"
                            : L"A  Play   ·   B  Library",
            64, 670, width - 128, 32, false, brushMuted_.Get());
        if (!status_.empty()) DrawTextLine(status_, 64, 716, width - 128, 42, false, brushMuted_.Get());
    } else if (page_ == Page::Import) {
        DrawTextLine(L"Import a game", 62, 154, 600, 60, true);
        DrawTextLine(L"Add a folder that contains a valid zero.manifest.json and native Windows game executable.",
            64, 214, width - 128, 48, false, brushMuted_.Get());
        ComPtr<ID2D1SolidColorBrush> white;
        target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
        const auto button = D2D1::RectF(62, 310, 420, 390);
        DrawRoundedCard(button, 22, brushAccent_.Get());
        DrawFocusRing(button, 22, true);
        DrawTextLine(L"A   Choose game folder", 100, 334, 280, 32, false, white.Get());
        DrawTextLine(L"ZERO validates, hashes, stages, verifies, and installs the package.",
            64, 430, width - 128, 48, false, brushMuted_.Get());
    } else if (page_ == Page::Settings) {
        const auto profile = identity_.CurrentProfile();
        DrawTextLine(L"Settings", 62, 154, 500, 60, true);
        DrawRoundedCard(D2D1::RectF(62, 270, width - 62, 355), 20, brushCard_.Get());
        DrawTextLine(L"Profile", 92, 290, 180, 30, false);
        DrawTextLine(Widen(profile.displayName.empty() ? settings_.profileName : profile.displayName), width - 420, 290, 320, 30, false, brushMuted_.Get());
        DrawRoundedCard(D2D1::RectF(62, 375, width - 62, 460), 20, brushCard_.Get());
        DrawTextLine(L"Zero ID", 92, 394, 180, 30, false);
        DrawTextLine(profile.zeroId.empty() ? L"Unavailable" : Widen(profile.zeroId), 300, 394, width - 580, 30, false, brushMuted_.Get());
        DrawTextLine(profile.localOnly ? L"Local only" : L"Connected", width - 220, 394, 120, 30, false, brushMuted_.Get());
        const auto motionRect = D2D1::RectF(62, 480, width - 62, 565);
        DrawRoundedCard(motionRect, 20, brushCard_.Get());
        DrawFocusRing(motionRect, 20);
        DrawTextLine(L"Reduced Motion", 92, 500, 220, 30, false);
        DrawTextLine(settings_.reducedMotion ? L"On" : L"Off", width - 240, 500, 100, 30, false, brushMuted_.Get());
        DrawTextLine(L"A Toggle", width - 380, 500, 110, 30, false, brushMuted_.Get());
    }

    target_->SetTransform(D2D1::Matrix3x2F::Identity());
    if (!status_.empty() && page_ != Page::GameDetail)
        DrawTextLine(status_, 64, height - 110, width - 130, 38, false, brushMuted_.Get());

    DrawOverlay(width, height);
    DrawCaptureViewer(width, height);
    DrawCaptureDeleteConfirm(width, height);

    const HRESULT result = target_->EndDraw();
    if (result == D2DERR_RECREATE_TARGET) {
        target_.Reset();
        cachedHero_.Reset();
        cachedHeroPath_.clear();
        cachedCapture_.Reset();
        cachedCapturePath_.clear();
    }
}

} // namespace zero
