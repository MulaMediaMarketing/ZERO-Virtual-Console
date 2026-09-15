#include "App.h"
#include "PackageTrustPresentation.h"
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

std::wstring pageTitle(ProductionUxPage page) {
    switch (page) {
        case ProductionUxPage::Discover: return L"Discover";
        case ProductionUxPage::CloudPlay: return L"Cloud Play";
        case ProductionUxPage::Downloads: return L"Downloads";
        case ProductionUxPage::Profile: return L"Profile";
        case ProductionUxPage::Devices: return L"Devices";
        case ProductionUxPage::Wishlist: return L"Wishlist";
        case ProductionUxPage::Checkout: return L"Checkout";
        case ProductionUxPage::Notifications: return L"Notifications";
        default: return L"ZERO Player";
    }
}

std::wstring pageDisconnectedMessage(ProductionUxPage page) {
    switch (page) {
        case ProductionUxPage::CloudPlay:
            return L"ZERO Cloud is not connected. Cloud sessions, regions, queue state, and latency are hidden until authoritative cloud service data is available.";
        case ProductionUxPage::Wishlist:
            return L"Wishlist requires the ZERO commerce service. No products are synthesized while commerce is disconnected.";
        case ProductionUxPage::Checkout:
            return L"Checkout requires a server-authoritative quote and payment flow. ZERO will not display a fake order or payment state.";
        default:
            return L"This destination is currently unavailable.";
    }
}

} // namespace

void App::DrawRecentGames(float width, float height) {
    const auto& games = registry_.Games();
    if (games.empty() || height < 780.0f) return;
    std::vector<size_t> order(games.size());
    for (size_t i = 0; i < games.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [this, &games](size_t a, size_t b) {
        return runtime_.PlatformState(games[a].packageId).lastPlayedAtUtc > runtime_.PlatformState(games[b].packageId).lastPlayedAtUtc;
    });
    DrawTextLine(L"RECENTLY PLAYED", 64, 650, 280, 30, false, brushMuted_.Get());
    const size_t count = std::min<size_t>(4, order.size());
    const float gap = 14.0f;
    const float cardWidth = (width - 128.0f - gap * 3.0f) / 4.0f;
    for (size_t i = 0; i < count; ++i) {
        const auto& game = games[order[i]];
        const float x = 64.0f + static_cast<float>(i) * (cardWidth + gap);
        const auto card = D2D1::RectF(x, 690, x + cardWidth, 755);
        DrawRoundedCard(card, 16, brushCard_.Get());
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
    const auto& products = store_.Products();
    ClampStoreSelection(storeUx_, store_.State(), products.size());
    DrawTextLine(L"Store", 62, 154, 500, 60, true);
    DrawTextLine(L"ZERO STORE", 64, 204, 300, 30, false, brushMuted_.Get());
    if (storeUx_.mode == StoreExperienceMode::Disconnected) {
        DrawEmptyState(L"Store service is not connected",
                       L"Catalog, checkout, entitlement, CDN, and publisher services appear here only when a real ZERO Store provider is connected.", width);
        return;
    }
    if (storeUx_.mode == StoreExperienceMode::Error) {
        DrawEmptyState(L"ZERO Store is unavailable", L"Your installed library remains available while the Store service recovers.", width);
        return;
    }
    if (storeUx_.mode == StoreExperienceMode::Empty) {
        DrawEmptyState(L"No products available", L"The Store provider is connected but returned no catalog entries.", width);
        return;
    }
    const size_t end = std::min(products.size(), storeUx_.scrollOffset + storeUx_.visibleRows);
    float y = 280.0f;
    for (size_t i = storeUx_.scrollOffset; i < end; ++i, y += 80.0f) {
        const auto& product = products[i];
        const auto rect = D2D1::RectF(62, y, width - 62, y + 64);
        DrawRoundedCard(rect, 18, brushCard_.Get());
        if (i == storeUx_.selectedIndex) DrawFocusRing(rect, 18);
        DrawTextLine(Widen(product.title), 92, y + 10, 430, 30, false);
        DrawTextLine(Widen(product.shortDescription), 92, y + 34, width - 520, 24, false, brushMuted_.Get());
        std::wstring right = StoreProductIsOwned(product) ? L"Owned" : Widen(product.displayPrice);
        DrawTextLine(right, width - 260, y + 18, 170, 30, false, brushMuted_.Get());
    }
}

void App::DrawFriends(float width, float height) {
    (void)height;
    const auto profile = identity_.CurrentProfile();
    const auto& list = friends_.Friends();
    ClampFriendsExperience(friendsUx_, friends_.State(), list);
    DrawTextLine(L"Friends", 62, 154, 500, 60, true);
    DrawTextLine(L"ZERO LINK", 64, 204, 300, 30, false, brushMuted_.Get());
    DrawRoundedCard(D2D1::RectF(62, 265, width - 62, 370), 24, brushCard_.Get());
    DrawTextLine(profile.displayName.empty() ? L"Player" : Widen(profile.displayName), 92, 286, 420, 42, true);
    DrawTextLine(profile.zeroId.empty() ? L"Local identity unavailable" : Widen(profile.zeroId), 92, 330, width - 360, 30, false, brushMuted_.Get());
    DrawTextLine(profile.localOnly ? L"LOCAL" : L"ONLINE", width - 240, 298, 150, 30, false, brushMuted_.Get());
    if (friendsUx_.mode == FriendsExperienceMode::Disconnected) {
        DrawEmptyState(L"Zero Link is disconnected", L"Presence, requests, parties, invites, and joinability appear only when the real social provider is connected.", width);
        return;
    }
    if (friendsUx_.mode == FriendsExperienceMode::Error) {
        DrawEmptyState(L"Zero Link is unavailable", L"The social provider reported an error. No synthetic social state is displayed.", width);
        return;
    }
    if (friendsUx_.mode == FriendsExperienceMode::Empty) {
        DrawEmptyState(L"No friends online", L"Zero Link is connected but returned no friend or presence records.", width);
        return;
    }
    const size_t end = std::min(list.size(), friendsUx_.scrollOffset + friendsUx_.visibleRows);
    float y = 405.0f;
    for (size_t i = friendsUx_.scrollOffset; i < end; ++i, y += 66.0f) {
        const auto& friendItem = list[i];
        const auto rect = D2D1::RectF(62, y, width - 62, y + 52);
        DrawRoundedCard(rect, 16, brushCard_.Get());
        if (i == friendsUx_.selectedIndex) DrawFocusRing(rect, 16);
        DrawTextLine(Widen(friendItem.displayName), 90, y + 10, 360, 30, false);
        std::wstring presence = L"Offline";
        if (friendItem.presence == PresenceState::Online) presence = L"Online";
        else if (friendItem.presence == PresenceState::InGame) presence = L"In game";
        if (FriendCanJoin(friendItem)) presence += L" · Joinable";
        DrawTextLine(presence, width - 500, y + 10, 410, 30, false, brushMuted_.Get());
    }
}

void App::DrawCaptures(float width, float height) {
    (void)height;
    const auto& items = captures_.Items();
    ClampCaptureExperience(capturesUx_, items);
    DrawTextLine(L"Capture", 62, 154, 500, 60, true);
    DrawTextLine(std::to_wstring(items.size()) + L" LOCAL CAPTURES   ·   A OPEN   ·   X DELETE   ·   RIGHT REVEAL", 64, 204, width - 128, 30, false, brushMuted_.Get());
    if (capturesUx_.mode == CaptureExperienceMode::Empty) {
        DrawEmptyState(L"No captures yet", L"Screenshots and clips created through ZERO appear here. No sample media is injected.", width);
        return;
    }
    const size_t end = std::min(items.size(), capturesUx_.scrollOffset + capturesUx_.visibleRows);
    float y = 270.0f;
    for (size_t i = capturesUx_.scrollOffset; i < end; ++i, y += 78.0f) {
        const auto& item = items[i];
        const auto rect = D2D1::RectF(62, y, width - 62, y + 62);
        DrawRoundedCard(rect, 18, i == capturesUx_.selectedIndex ? brushAccent_.Get() : brushCard_.Get());
        if (i == capturesUx_.selectedIndex) DrawFocusRing(rect, 18);
        DrawTextLine(item.path.filename().wstring(), 92, y + 10, width - 430, 30, false);
        DrawTextLine(captureKindLabel(item.kind) + L"  ·  " + fileSizeLabel(item.sizeBytes), width - 385, y + 10, 300, 30, false, brushMuted_.Get());
    }
}

void App::DrawCaptureViewer(float width, float height) {
    if (capturesUx_.mode != CaptureExperienceMode::Viewer) return;
    const auto& items = captures_.Items();
    const auto selected = SelectedCaptureIndex(capturesUx_, items);
    if (!selected) return;
    const auto& item = items[*selected];
    ComPtr<ID2D1SolidColorBrush> veil;
    ComPtr<ID2D1SolidColorBrush> white;
    target_->CreateSolidColorBrush(D2D1::ColorF(0x000000, 0.96f), veil.GetAddressOf());
    target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
    target_->FillRectangle(D2D1::RectF(0, 0, width, height), veil.Get());
    if (cachedCapturePath_ != item.path) {
        cachedCapture_.Reset();
        cachedCapturePath_ = item.path;
        cachedCapture_ = LoadBitmap(item.path);
    }
    if (cachedCapture_) {
        const auto size = cachedCapture_->GetSize();
        const float scale = std::min((width - 160.0f) / size.width, (height - 180.0f) / size.height);
        const float drawWidth = size.width * scale;
        const float drawHeight = size.height * scale;
        const float x = (width - drawWidth) * 0.5f;
        const float y = (height - drawHeight) * 0.5f - 10.0f;
        target_->DrawBitmap(cachedCapture_.Get(), D2D1::RectF(x, y, x + drawWidth, y + drawHeight), 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    } else {
        DrawTextLine(L"ZERO could not decode this screenshot.", 80, height * 0.45f, width - 160, 50, true, white.Get());
    }
    DrawTextLine(item.path.filename().wstring(), 64, 34, width - 128, 36, false, white.Get());
    DrawTextLine(L"B CLOSE   ·   X DELETE   ·   RIGHT REVEAL", 64, height - 58, width - 128, 30, false, white.Get());
}

void App::DrawCaptureDeleteConfirm(float width, float height) {
    if (capturesUx_.mode != CaptureExperienceMode::DeleteConfirm) return;
    const auto& items = captures_.Items();
    const auto selected = SelectedCaptureIndex(capturesUx_, items);
    if (!selected) return;
    ComPtr<ID2D1SolidColorBrush> veil;
    target_->CreateSolidColorBrush(D2D1::ColorF(0x000000, 0.76f), veil.GetAddressOf());
    target_->FillRectangle(D2D1::RectF(0, 0, width, height), veil.Get());
    const float left = std::max(120.0f, width * 0.5f - 330.0f);
    const float top = std::max(120.0f, height * 0.5f - 150.0f);
    DrawRoundedCard(D2D1::RectF(left, top, left + 660, top + 300), 28, brushCard_.Get());
    DrawTextLine(L"Delete capture?", left + 40, top + 40, 520, 50, true);
    DrawTextLine(items[*selected].path.filename().wstring(), left + 40, top + 105, 580, 34, false);
    DrawTextLine(L"This permanently removes the local capture file.", left + 40, top + 150, 580, 42, false, brushMuted_.Get());
    const auto action = D2D1::RectF(left + 38, top + 214, left + 238, top + 264);
    DrawRoundedCard(action, 16, brushAccent_.Get());
    DrawFocusRing(action, 16);
    DrawTextLine(L"A   DELETE", left + 72, top + 227, 130, 26, false);
    DrawTextLine(L"B CANCEL", left + 280, top + 227, 140, 26, false, brushMuted_.Get());
}

void App::DrawOverlay(float width, float height) {
    if (!overlayVisible_) return;
    const float opacity = shellUx_.OverlayOpacity();
    const float slide = shellUx_.OverlayOffsetX();
    ComPtr<ID2D1SolidColorBrush> veil;
    ComPtr<ID2D1SolidColorBrush> panel;
    target_->CreateSolidColorBrush(D2D1::ColorF(0x000000, 0.68f * opacity), veil.GetAddressOf());
    target_->CreateSolidColorBrush(D2D1::ColorF(0x0B111C, 0.99f), panel.GetAddressOf());
    target_->FillRectangle(D2D1::RectF(0, 0, width, height), veil.Get());
    const float left = width - 470.0f + slide;
    DrawRoundedCard(D2D1::RectF(left, 40, width - 40 + slide, height - 40), 28, panel.Get());

    if (achievementsFromOverlay_) {
        DrawTextLine(L"Achievements", left + 34, 75, 340, 48, true);
        DrawTextLine(Widen(achievementPackageId_), left + 36, 122, 350, 28, false, brushMuted_.Get());
        const auto records = achievementPackageId_.empty() ? std::vector<AchievementRecord>{} : runtime_.Achievements(achievementPackageId_);
        if (records.empty()) {
            DrawTextLine(L"No achievements unlocked yet.", left + 36, 205, 340, 34, false);
        } else {
            const size_t end = std::min(records.size(), achievementScroll_ + 6);
            float y = 180.0f;
            for (size_t i = achievementScroll_; i < end; ++i, y += 76.0f) {
                const auto rect = D2D1::RectF(left + 26, y, width - 66 + slide, y + 60);
                if (i == selectedAchievement_) {
                    DrawRoundedCard(rect, 16, brushAccent_.Get());
                    DrawFocusRing(rect, 16);
                }
                DrawTextLine(records[i].title.empty() ? Widen(records[i].id) : Widen(records[i].title), left + 48, y + 8, 300, 26, false);
                DrawTextLine(Widen(records[i].unlockedAtUtc), left + 48, y + 32, 300, 22, false, brushMuted_.Get());
            }
        }
        DrawTextLine(L"UP/DOWN BROWSE   B BACK", left + 36, height - 92, 340, 30, false, brushMuted_.Get());
        return;
    }

    DrawTextLine(L"ZERO", left + 34, 75, 250, 48, true);
    DrawTextLine(L"SYSTEM OVERLAY", left + 36, 119, 280, 28, false, brushMuted_.Get());
    std::wstring session = L"No active session";
    if (runtime_.IsActive() || runtime_.State() == RuntimeState::Running) session = Widen(runtime_.Info().title);
    DrawTextLine(session, left + 36, 170, 350, 34, false);
    DrawTextLine(L"Playtime   " + std::to_wstring(runtime_.PlaytimeSeconds() / 60) + L" min", left + 36, 210, 350, 30, false, brushMuted_.Get());
    const wchar_t* options[] = {L"Continue Game", L"Friends", L"Capture", L"Achievements", L"Exit Game"};
    for (size_t i = 0; i < 5; ++i) {
        const float y = 305.0f + static_cast<float>(i) * 66.0f;
        const auto rect = D2D1::RectF(left + 26, y, width - 66 + slide, y + 50);
        if (overlayIndex_ == i) {
            DrawRoundedCard(rect, 16, brushAccent_.Get());
            DrawFocusRing(rect, 16);
        }
        DrawTextLine(options[i], left + 48, y + 11, 300, 30, false);
    }
    DrawTextLine(L"A SELECT   B CLOSE", left + 36, height - 92, 320, 30, false, brushMuted_.Get());
}

void App::Paint() {
    CreateDeviceResources();
    if (!target_) return;

    brushText_->SetColor(D2D1::ColorF(0xF4F8FF));
    brushMuted_->SetColor(D2D1::ColorF(0x8C9AAF));
    brushAccent_->SetColor(D2D1::ColorF(0x087DFF));
    brushCard_->SetColor(D2D1::ColorF(0x121A27));

    target_->BeginDraw();
    target_->SetTransform(D2D1::Matrix3x2F::Identity());
    target_->Clear(D2D1::ColorF(0x060A11));

    RECT rc{};
    GetClientRect(hwnd_, &rc);
    const float width = static_cast<float>(rc.right);
    const float height = static_cast<float>(rc.bottom);
    constexpr float sidebarWidth = 230.0f;
    constexpr float topBarHeight = 94.0f;
    const float contentWidth = std::max(720.0f, width - sidebarWidth);

    ComPtr<ID2D1SolidColorBrush> sidebar;
    ComPtr<ID2D1SolidColorBrush> topbar;
    ComPtr<ID2D1SolidColorBrush> selected;
    target_->CreateSolidColorBrush(D2D1::ColorF(0x090E17), sidebar.GetAddressOf());
    target_->CreateSolidColorBrush(D2D1::ColorF(0x0A101A), topbar.GetAddressOf());
    target_->CreateSolidColorBrush(D2D1::ColorF(0x0C5FDB), selected.GetAddressOf());
    target_->FillRectangle(D2D1::RectF(0, 0, sidebarWidth, height), sidebar.Get());
    target_->FillRectangle(D2D1::RectF(sidebarWidth, 0, width, topBarHeight), topbar.Get());

    DrawTextLine(L"ZERO", 28, 24, 160, 48, true);
    DrawTextLine(L"PLAYER", 30, 64, 160, 24, false, brushMuted_.Get());

    const bool topLevel = ProductionUxIsTopLevelPage(page_);
    float navY = 126.0f;
    for (size_t i = 0; i < ProductionUxNavCount(); ++i, navY += 52.0f) {
        const auto rect = D2D1::RectF(16, navY, sidebarWidth - 16, navY + 42);
        if (navIndex_ == i) {
            DrawRoundedCard(rect, 13, selected.Get());
            if (topLevel) DrawFocusRing(rect, 13);
        }
        DrawTextLine(std::wstring(kProductionUxNavigation[i].label), 34, navY + 9, 168, 28, false,
                     navIndex_ == i ? brushText_.Get() : brushMuted_.Get());
    }

    const auto profile = identity_.CurrentProfile();
    DrawTextLine(L"SEARCH", sidebarWidth + 34, 32, 100, 28, false, brushMuted_.Get());
    DrawTextLine(L"NOTIFICATIONS", width - 510, 32, 150, 28, false, brushMuted_.Get());
    DrawTextLine(L"DOWNLOADS", width - 340, 32, 110, 28, false, brushMuted_.Get());
    DrawTextLine(profile.displayName.empty() ? L"PLAYER" : Widen(profile.displayName), width - 210, 27, 180, 30, false);
    DrawTextLine(profile.localOnly ? L"LOCAL ZERO ID" : L"ZERO ID ONLINE", width - 210, 55, 180, 22, false, brushMuted_.Get());

    target_->SetTransform(D2D1::Matrix3x2F::Translation(sidebarWidth, shellUx_.PageOffsetY()));
    const auto& games = registry_.Games();
    ClampCoreShellSelection(coreShellUx_, games.size());

    if (page_ == Page::Home) {
        const std::string greetingName = profile.displayName.empty() ? settings_.profileName : profile.displayName;
        DrawTextLine(L"Welcome back, " + Widen(greetingName), 62, 132, 700, 60, true);
        DrawTextLine(L"YOUR ZERO PLAYER", 64, 190, 520, 30, false, brushMuted_.Get());
        if (coreShellUx_.homeMode == HomeExperienceMode::Empty) {
            DrawEmptyState(L"No games installed", L"Open Library and import a validated ZERO-compatible game package.", contentWidth);
        } else {
            const auto& game = games[coreShellUx_.selectedGame];
            const auto trustUi = PresentPackageTrust(runtime_.PackageTrust(game));
            const auto hero = D2D1::RectF(62, 250, contentWidth - 62, 620);
            DrawHeroArtwork(game, hero);
            ComPtr<ID2D1SolidColorBrush> shade;
            target_->CreateSolidColorBrush(D2D1::ColorF(0x000000, 0.58f), shade.GetAddressOf());
            target_->FillRectangle(hero, shade.Get());
            DrawTextLine(Widen(game.title), 108, 322, 760, 60, true);
            DrawTextLine(L"Version " + Widen(game.version) + L"   ·   " + trustUi.shortLabel, 110, 388, 650, 35, false, brushMuted_.Get());
            const auto resume = game.zeroResume ? resumeStore_.Load(game.packageId) : std::nullopt;
            const auto playRect = D2D1::RectF(108, 500, 300, 556);
            DrawRoundedCard(playRect, 20, brushAccent_.Get());
            DrawFocusRing(playRect, 20);
            DrawTextLine(resume ? L"RESUME" : L"PLAY", 154, 515, 120, 30, false);
            if (resume) DrawTextLine(L"Continue: " + Widen(resume->displayLabel), 330, 515, 500, 30, false);
            DrawRecentGames(contentWidth, height);
        }
    } else if (page_ == Page::Discover) DrawDiscover(contentWidth, height);
    else if (page_ == Page::Library) {
        DrawTextLine(L"Library", 62, 132, 500, 60, true);
        DrawTextLine(std::to_wstring(games.size()) + L" INSTALLED   ·   A OPEN   ·   X IMPORT GAME", 64, 190, 620, 30, false, brushMuted_.Get());
        if (coreShellUx_.libraryMode == LibraryExperienceMode::Empty) {
            DrawEmptyState(L"Your library is empty", L"Press X to import a validated ZERO-compatible native game package.", contentWidth);
        } else {
            float y = 260.0f;
            const size_t count = std::min<size_t>(8, games.size());
            for (size_t i = 0; i < count; ++i, y += 78.0f) {
                const auto rect = D2D1::RectF(62, y, contentWidth - 62, y + 62);
                DrawRoundedCard(rect, 18, brushCard_.Get());
                if (i == coreShellUx_.selectedGame) DrawFocusRing(rect, 18);
                DrawTextLine(Widen(games[i].title), 92, y + 9, 540, 28, false);
                DrawTextLine(Widen(games[i].version), contentWidth - 250, y + 14, 120, 30, false, brushMuted_.Get());
            }
        }
    } else if (page_ == Page::Downloads) DrawDownloads(contentWidth, height);
    else if (page_ == Page::Store) DrawStore(contentWidth, height);
    else if (page_ == Page::Friends) DrawFriends(contentWidth, height);
    else if (page_ == Page::Captures) DrawCaptures(contentWidth, height);
    else if (page_ == Page::Profile) DrawProfile(contentWidth, height);
    else if (page_ == Page::Devices) DrawDevices(contentWidth, height);
    else if (page_ == Page::Notifications) DrawNotifications(contentWidth, height);
    else if (page_ == Page::Settings) DrawSettings(contentWidth, height);
    else if (page_ == Page::Achievements) DrawAchievements(contentWidth, height);
    else if (page_ == Page::GameDetail && !games.empty()) {
        const auto& game = games[coreShellUx_.selectedGame];
        const auto platform = runtime_.PlatformState(game.packageId);
        const auto achievements = runtime_.Achievements(game.packageId);
        const auto resume = game.zeroResume ? resumeStore_.Load(game.packageId) : std::nullopt;
        const auto trustUi = PresentPackageTrust(runtime_.PackageTrust(game));
        DrawTextLine(Widen(game.title), 62, 132, contentWidth - 124, 60, true);
        DrawTextLine(Widen(game.packageId), 64, 190, contentWidth - 128, 30, false, brushMuted_.Get());
        const auto hero = D2D1::RectF(62, 245, contentWidth - 430, 610);
        DrawHeroArtwork(game, hero);
        const auto info = D2D1::RectF(contentWidth - 400, 245, contentWidth - 62, 610);
        DrawRoundedCard(info, 24, brushCard_.Get());
        DrawTextLine(L"GAME STATUS", contentWidth - 370, 275, 260, 34, false, brushMuted_.Get());
        DrawTextLine(L"Playtime   " + std::to_wstring(platform.totalPlaytimeSeconds / 60) + L" min", contentWidth - 370, 330, 280, 30, false);
        DrawTextLine(L"Launches   " + std::to_wstring(platform.launchCount), contentWidth - 370, 372, 280, 30, false);
        DrawTextLine(L"Achievements   " + std::to_wstring(achievements.size()), contentWidth - 370, 414, 280, 30, false);
        DrawTextLine(L"Trust   " + trustUi.shortLabel, contentWidth - 370, 456, 280, 30, false);
        const auto action = D2D1::RectF(contentWidth - 370, 520, contentWidth - 190, 574);
        DrawRoundedCard(action, 18, brushAccent_.Get());
        DrawFocusRing(action, 18);
        DrawTextLine(resume && preferResume_ ? L"RESUME" : L"PLAY", contentWidth - 325, 535, 110, 28, false);
    } else if (page_ == Page::Import) {
        DrawTextLine(L"Import Game", 62, 132, 600, 60, true);
        DrawTextLine(L"VALIDATED LOCAL PACKAGE INSTALL", 64, 190, 520, 30, false, brushMuted_.Get());
        DrawEmptyState(L"Choose a game folder", L"ZERO validates the manifest, package integrity, executable path, and installation root before adding it to Library.", contentWidth);
        const auto button = D2D1::RectF(100, 455, 410, 515);
        DrawRoundedCard(button, 20, brushAccent_.Get());
        DrawFocusRing(button, 20);
        DrawTextLine(L"A   CHOOSE FOLDER", 145, 472, 230, 30, false);
    } else {
        DrawTextLine(pageTitle(page_), 62, 132, 600, 60, true);
        DrawTextLine(L"ZERO PLAYER SERVICE", 64, 190, 520, 30, false, brushMuted_.Get());
        DrawEmptyState(pageTitle(page_) + L" is currently disconnected", pageDisconnectedMessage(page_), contentWidth);
    }

    target_->SetTransform(D2D1::Matrix3x2F::Identity());
    if (!status_.empty() && page_ != Page::GameDetail && page_ != Page::Achievements)
        DrawTextLine(status_, sidebarWidth + 64, height - 72, width - sidebarWidth - 130, 38, false, brushMuted_.Get());

    DrawOverlay(width, height);
    DrawCaptureViewer(width, height);
    DrawCaptureDeleteConfirm(width, height);
    DrawLaunchRecovery(width, height);

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
