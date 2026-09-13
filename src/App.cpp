#include "App.h"
#include <shlobj.h>
#include <shobjidl.h>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace zero {
namespace {
std::filesystem::path localRoot() {
    PWSTR p = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &p))) {
        std::filesystem::path out = std::filesystem::path(p) / "ZERO";
        CoTaskMemFree(p);
        return out;
    }
    return std::filesystem::current_path() / "ZeroData";
}
}

App::App(HINSTANCE instance)
    : instance_(instance),
      registry_(localRoot() / "Library"),
      importer_(localRoot() / "Library"),
      captures_(localRoot() / "Captures"),
      settingsStore_(localRoot()) {}

int App::Run() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    settings_ = settingsStore_.Load();
    registry_.Refresh();
    captures_.Refresh();
    if (!InitWindow() || !InitGraphics()) return 1;
    ShowWindow(hwnd_, SW_SHOW);
    EnterBorderlessFullscreen();
    UpdateWindow(hwnd_);
    SetTimer(hwnd_, 1, 16, nullptr);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    settingsStore_.Save(settings_);
    CoUninitialize();
    return static_cast<int>(msg.wParam);
}

bool App::InitWindow() {
    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance_;
    wc.lpszClassName = L"ZeroVirtualConsoleWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);
    hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, L"ZERO Virtual Console",
        WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 1600, 900,
        nullptr, nullptr, instance_, this);
    return hwnd_ != nullptr;
}

void App::EnterBorderlessFullscreen() {
    if (!hwnd_) return;
    HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(monitor, &info)) return;
    SetWindowLongPtrW(hwnd_, GWL_STYLE, WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    SetWindowPos(hwnd_, HWND_TOP, info.rcMonitor.left, info.rcMonitor.top,
        info.rcMonitor.right - info.rcMonitor.left,
        info.rcMonitor.bottom - info.rcMonitor.top,
        SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

bool App::InitGraphics() {
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory_.GetAddressOf()))) return false;
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(writeFactory_.GetAddressOf())))) return false;
    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(wicFactory_.GetAddressOf()));
    writeFactory_->CreateTextFormat(L"Segoe UI Variable Display", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 34.0f, L"en-us", heading_.GetAddressOf());
    writeFactory_->CreateTextFormat(L"Segoe UI Variable Text", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 18.0f, L"en-us", body_.GetAddressOf());
    heading_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    body_->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
    return true;
}

void App::CreateDeviceResources() {
    if (target_) return;
    RECT rc{}; GetClientRect(hwnd_, &rc);
    auto size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
    d2dFactory_->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd_, size), target_.GetAddressOf());
    if (!target_) return;
    target_->CreateSolidColorBrush(D2D1::ColorF(0x141414), brushText_.GetAddressOf());
    target_->CreateSolidColorBrush(D2D1::ColorF(0x6E6E6E), brushMuted_.GetAddressOf());
    target_->CreateSolidColorBrush(D2D1::ColorF(0x151515), brushAccent_.GetAddressOf());
    target_->CreateSolidColorBrush(D2D1::ColorF(0xF2F0EB), brushCard_.GetAddressOf());
}

ComPtr<ID2D1Bitmap> App::LoadBitmap(const std::filesystem::path& path) {
    ComPtr<ID2D1Bitmap> bitmap;
    if (!target_ || !wicFactory_ || path.empty() || !std::filesystem::exists(path)) return bitmap;
    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(wicFactory_->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
        WICDecodeMetadataCacheOnLoad, decoder.GetAddressOf()))) return bitmap;
    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, frame.GetAddressOf()))) return bitmap;
    ComPtr<IWICFormatConverter> converter;
    if (FAILED(wicFactory_->CreateFormatConverter(converter.GetAddressOf()))) return bitmap;
    if (FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut))) return bitmap;
    target_->CreateBitmapFromWicBitmap(converter.Get(), nullptr, bitmap.GetAddressOf());
    return bitmap;
}

void App::DrawHeroArtwork(const GameManifest& game, const D2D1_RECT_F& bounds) {
    if (game.heroImage.empty()) {
        DrawRoundedCard(bounds, 30, brushCard_.Get());
        return;
    }
    if (cachedHeroPath_ != game.heroImage) {
        cachedHero_.Reset();
        cachedHeroPath_ = game.heroImage;
        cachedHero_ = LoadBitmap(game.heroImage);
    }
    if (!cachedHero_) {
        DrawRoundedCard(bounds, 30, brushCard_.Get());
        return;
    }

    const auto source = cachedHero_->GetSize();
    const float destW = bounds.right - bounds.left;
    const float destH = bounds.bottom - bounds.top;
    const float srcAspect = source.width / source.height;
    const float dstAspect = destW / destH;
    D2D1_RECT_F src = D2D1::RectF(0, 0, source.width, source.height);
    if (srcAspect > dstAspect) {
        const float cropW = source.height * dstAspect;
        const float x = (source.width - cropW) * 0.5f;
        src = D2D1::RectF(x, 0, x + cropW, source.height);
    } else if (srcAspect < dstAspect) {
        const float cropH = source.width / dstAspect;
        const float y = (source.height - cropH) * 0.5f;
        src = D2D1::RectF(0, y, source.width, y + cropH);
    }
    target_->DrawBitmap(cachedHero_.Get(), bounds, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, src);
}

void App::DrawTextLine(const std::wstring& s, float x, float y, float w, float h, bool head, ID2D1Brush* brush) {
    if (!target_) return;
    target_->DrawTextW(s.c_str(), static_cast<UINT32>(s.size()), head ? heading_.Get() : body_.Get(),
        D2D1::RectF(x,y,x+w,y+h), brush ? brush : brushText_.Get());
}

void App::DrawRoundedCard(const D2D1_RECT_F& r, float radius, ID2D1Brush* fill) {
    target_->FillRoundedRectangle(D2D1::RoundedRect(r, radius, radius), fill);
}

std::wstring App::Widen(const std::string& s) const {
    if (s.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (count <= 0) return std::wstring(s.begin(), s.end());
    std::wstring out(static_cast<size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), count);
    return out;
}

void App::DrawRecentGames(float W, float H) {
    const auto& games = registry_.Games();
    if (games.empty() || H < 780.0f) return;
    std::vector<size_t> order(games.size());
    for (size_t i = 0; i < games.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [this, &games](size_t a, size_t b) {
        return runtime_.PlatformState(games[a].packageId).lastPlayedAtUtc >
               runtime_.PlatformState(games[b].packageId).lastPlayedAtUtc;
    });
    DrawTextLine(L"Recently Played", 64, 650, 260, 30, false, brushMuted_.Get());
    const size_t count = std::min<size_t>(4, order.size());
    const float gap = 14.0f;
    const float cardW = (W - 128.0f - gap * 3.0f) / 4.0f;
    for (size_t i = 0; i < count; ++i) {
        const auto& game = games[order[i]];
        const float x = 64.0f + i * (cardW + gap);
        DrawRoundedCard(D2D1::RectF(x, 690, x + cardW, 755), 16, brushCard_.Get());
        DrawTextLine(Widen(game.title), x + 18, 710, cardW - 36, 30, false);
    }
}

void App::DrawEmptyState(const std::wstring& title, const std::wstring& body, float W) {
    DrawRoundedCard(D2D1::RectF(62,270,W-62,540), 28, brushCard_.Get());
    DrawTextLine(title, 100, 322, W-200, 52, true);
    DrawTextLine(body, 102, 390, W-230, 88, false, brushMuted_.Get());
}

void App::DrawCaptures(float W, float H) {
    (void)H;
    const auto& items = captures_.Items();
    DrawTextLine(L"Captures", 62, 154, 500, 60, true);
    DrawTextLine(std::to_wstring(items.size()) + L" local captures", 64, 204, 300, 30, false, brushMuted_.Get());
    if (items.empty()) {
        DrawEmptyState(L"No captures yet", L"Screenshots and clips created through ZERO will appear here. No sample captures are injected.", W);
        return;
    }

    float y = 280.0f;
    const size_t count = std::min<size_t>(7, items.size());
    for (size_t i = 0; i < count; ++i, y += 70.0f) {
        DrawRoundedCard(D2D1::RectF(62,y,W-62,y+54), 16, brushCard_.Get());
        DrawTextLine(items[i].path.filename().wstring(), 88, y+12, W-230, 30, false);
        DrawTextLine(items[i].path.extension().wstring(), W-180, y+12, 90, 30, false, brushMuted_.Get());
    }
}

void App::DrawOverlay(float W, float H) {
    if (!overlayVisible_) return;
    ComPtr<ID2D1SolidColorBrush> veil;
    ComPtr<ID2D1SolidColorBrush> panel;
    ComPtr<ID2D1SolidColorBrush> white;
    target_->CreateSolidColorBrush(D2D1::ColorF(0x000000, 0.62f), veil.GetAddressOf());
    target_->CreateSolidColorBrush(D2D1::ColorF(0x181818, 0.98f), panel.GetAddressOf());
    target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
    target_->FillRectangle(D2D1::RectF(0,0,W,H), veil.Get());

    const float left = W - 470.0f;
    DrawRoundedCard(D2D1::RectF(left, 40, W - 40, H - 40), 28, panel.Get());
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
        if (overlayIndex_ == i) {
            ComPtr<ID2D1SolidColorBrush> selected;
            target_->CreateSolidColorBrush(D2D1::ColorF(0x333333), selected.GetAddressOf());
            DrawRoundedCard(D2D1::RectF(left + 26, y, W - 66, y + 50), 16, selected.Get());
        }
        DrawTextLine(options[i], left + 48, y + 11, 300, 30, false, white.Get());
    }
    DrawTextLine(L"A Select   B Close", left + 36, H - 92, 320, 30, false, brushMuted_.Get());
}

void App::Paint() {
    CreateDeviceResources();
    if (!target_) return;
    target_->BeginDraw();
    target_->Clear(D2D1::ColorF(0xFBFAF7));
    RECT rc{}; GetClientRect(hwnd_, &rc);
    const float W = float(rc.right), H = float(rc.bottom);

    DrawTextLine(L"ZERO", 42, 28, 180, 54, true);
    DrawTextLine(L"VIRTUAL CONSOLE", 44, 68, 210, 28, false, brushMuted_.Get());

    const wchar_t* nav[] = {L"Home", L"Library", L"Store", L"Friends", L"Captures", L"Settings"};
    const float navStart = 300.0f;
    const float navStep = 128.0f;
    for (int i=0;i<6;i++) {
        const float x = navStart + i*navStep;
        if (navIndex_ == size_t(i)) DrawRoundedCard(D2D1::RectF(x-16,38,x+102,82), 18, brushCard_.Get());
        DrawTextLine(nav[i], x, 48, 104, 30, false);
    }

    const auto& games = registry_.Games();
    if (page_ == Page::Home) {
        DrawTextLine(L"Good evening, " + Widen(settings_.profileName), 62, 154, 700, 60, true);
        DrawTextLine(L"Your games. One calm console experience.", 64, 204, 650, 40, false, brushMuted_.Get());
        if (games.empty()) {
            DrawEmptyState(L"No games installed", L"Open Library, then use X to import a ZERO-compatible game folder.", W);
        } else {
            const auto& g = games[std::min(selectedGame_, games.size()-1)];
            const auto hero = D2D1::RectF(62,285,W-62,620);
            DrawHeroArtwork(g, hero);
            ComPtr<ID2D1SolidColorBrush> shade;
            ComPtr<ID2D1SolidColorBrush> white;
            target_->CreateSolidColorBrush(D2D1::ColorF(0x000000, 0.50f), shade.GetAddressOf());
            target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
            target_->FillRectangle(hero, shade.Get());
            DrawTextLine(Widen(g.title), 108, 334, 760, 60, true, white.Get());
            DrawTextLine(L"Version " + Widen(g.version), 110, 392, 400, 35, false, white.Get());
            const auto resume = g.zeroResume ? resumeStore_.Load(g.packageId) : std::nullopt;
            DrawRoundedCard(D2D1::RectF(108,500,300,556), 20, white.Get());
            DrawTextLine(resume ? L"Resume" : L"Play", 160, 515, 120, 30, false, brushText_.Get());
            if (resume) DrawTextLine(L"Continue: " + Widen(resume->displayLabel), 330, 515, 500, 30, false, white.Get());
            DrawRecentGames(W, H);
        }
    } else if (page_ == Page::Library) {
        DrawTextLine(L"Library", 62, 154, 500, 60, true);
        DrawTextLine(std::to_wstring(games.size()) + L" installed   ·   A Open   ·   X Import Game", 64, 204, 620, 30, false, brushMuted_.Get());
        if (games.empty()) {
            DrawEmptyState(L"Your library is empty", L"Press X to import a validated ZERO-compatible native game package.", W);
        } else {
            float y=280;
            for (size_t i=0;i<games.size() && i<8;i++,y+=78) {
                auto rect = D2D1::RectF(62,y,W-62,y+62);
                if (i==selectedGame_) DrawRoundedCard(rect, 18, brushCard_.Get());
                DrawTextLine(Widen(games[i].title), 92, y+14, 540, 30, false);
                DrawTextLine(Widen(games[i].version), W-250, y+14, 120, 30, false, brushMuted_.Get());
            }
        }
    } else if (page_ == Page::Store) {
        DrawTextLine(L"Store", 62, 154, 500, 60, true);
        DrawTextLine(L"ZERO Store", 64, 204, 300, 30, false, brushMuted_.Get());
        DrawEmptyState(L"Store service is not connected", L"This shell destination is production-wired, but commerce, entitlement, CDN, and publisher catalog services are intentionally not being faked in this build.", W);
    } else if (page_ == Page::Friends) {
        DrawTextLine(L"Friends", 62, 154, 500, 60, true);
        DrawTextLine(L"Zero Link", 64, 204, 300, 30, false, brushMuted_.Get());
        DrawEmptyState(L"Friends service is not connected", L"Zero Link presence, requests, joinability, invites, and messaging will populate this page when the social provider is implemented. No fake friends are shown.", W);
    } else if (page_ == Page::Captures) {
        DrawCaptures(W, H);
    } else if (page_ == Page::GameDetail && !games.empty()) {
        const auto& g = games[std::min(selectedGame_, games.size()-1)];
        const auto platform = runtime_.PlatformState(g.packageId);
        const auto achievements = runtime_.Achievements(g.packageId);
        const auto resume = g.zeroResume ? resumeStore_.Load(g.packageId) : std::nullopt;
        const float split = std::max(760.0f, W - 470.0f);
        const auto hero = D2D1::RectF(62,150,split,535);

        DrawHeroArtwork(g, hero);
        ComPtr<ID2D1SolidColorBrush> shade;
        ComPtr<ID2D1SolidColorBrush> white;
        target_->CreateSolidColorBrush(D2D1::ColorF(0x000000, 0.48f), shade.GetAddressOf());
        target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
        target_->FillRectangle(hero, shade.Get());

        DrawTextLine(Widen(g.title), 104, 205, split-150, 58, true, white.Get());
        DrawTextLine(Widen(g.packageId), 106, 263, split-170, 32, false, white.Get());
        DrawTextLine(L"Version " + Widen(g.version), 106, 300, 380, 30, false, white.Get());

        DrawRoundedCard(D2D1::RectF(104,430,282,488), 20,
            preferResume_ && resume ? brushCard_.Get() : white.Get());
        DrawTextLine(L"Play", 166, 446, 90, 30, false, brushText_.Get());
        if (resume) {
            DrawRoundedCard(D2D1::RectF(300,430,500,488), 20,
                preferResume_ ? white.Get() : brushCard_.Get());
            DrawTextLine(L"Resume", 354, 446, 110, 30, false, brushText_.Get());
        }

        const float cardLeft = split + 20.0f;
        DrawRoundedCard(D2D1::RectF(cardLeft,150,W-62,535), 26, brushCard_.Get());
        DrawTextLine(L"Game Status", cardLeft+28, 180, 260, 42, true);
        DrawTextLine(L"Playtime", cardLeft+28, 245, 160, 30, false, brushMuted_.Get());
        DrawTextLine(std::to_wstring(platform.totalPlaytimeSeconds / 60) + L" min", W-250, 245, 150, 30, false);
        DrawTextLine(L"Launches", cardLeft+28, 285, 160, 30, false, brushMuted_.Get());
        DrawTextLine(std::to_wstring(platform.launchCount), W-250, 285, 150, 30, false);
        DrawTextLine(L"Achievements", cardLeft+28, 325, 180, 30, false, brushMuted_.Get());
        DrawTextLine(std::to_wstring(achievements.size()) + L" unlocked", W-250, 325, 150, 30, false);
        DrawTextLine(L"Resume", cardLeft+28, 365, 160, 30, false, brushMuted_.Get());
        const std::wstring resumeState = !g.zeroResume ? L"Not supported" : (resume ? L"Checkpoint ready" : L"No checkpoint");
        DrawTextLine(resumeState, W-280, 365, 180, 30, false);
        DrawTextLine(L"Last session", cardLeft+28, 405, 160, 30, false, brushMuted_.Get());
        const std::wstring lastSession = platform.launchCount == 0 ? L"Never played" :
            (platform.lastSessionCrashed ? L"Ended unexpectedly" : L"Clean exit");
        DrawTextLine(lastSession, W-280, 405, 180, 30, false);
        DrawTextLine(L"Last played", cardLeft+28, 445, 160, 30, false, brushMuted_.Get());
        DrawTextLine(platform.lastPlayedAtUtc.empty() ? L"—" : Widen(platform.lastPlayedAtUtc), W-330, 445, 230, 30, false);
        DrawTextLine(L"Exit code", cardLeft+28, 485, 160, 30, false, brushMuted_.Get());
        DrawTextLine(std::to_wstring(platform.lastExitCode), W-250, 485, 150, 30, false);

        if (resume) {
            DrawRoundedCard(D2D1::RectF(62,565,W-62,635), 20, brushCard_.Get());
            DrawTextLine(L"Continue from", 88, 584, 180, 30, false, brushMuted_.Get());
            DrawTextLine(Widen(resume->displayLabel), 270, 584, W-360, 30, false);
        }
        DrawTextLine(resume ? L"Left / Right  Choose Play or Resume   ·   A  Launch   ·   B  Library"
                            : L"A  Play   ·   B  Library",
                     64, 670, W-128, 32, false, brushMuted_.Get());
        if (!status_.empty()) DrawTextLine(status_, 64, 716, W-128, 42, false, brushMuted_.Get());
    } else if (page_ == Page::Import) {
        DrawTextLine(L"Import a game", 62, 154, 600, 60, true);
        DrawTextLine(L"Add a folder that contains a valid zero.manifest.json and native Windows game executable.",
            64, 214, W-128, 48, false, brushMuted_.Get());
        DrawRoundedCard(D2D1::RectF(62,310,420,390), 22, brushAccent_.Get());
        ComPtr<ID2D1SolidColorBrush> white; target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
        DrawTextLine(L"A   Choose game folder", 100, 334, 280, 32, false, white.Get());
        DrawTextLine(L"ZERO validates, hashes, stages, verifies, and installs the package.",
            64, 430, W-128, 48, false, brushMuted_.Get());
    } else if (page_ == Page::Settings) {
        DrawTextLine(L"Settings", 62, 154, 500, 60, true);
        DrawRoundedCard(D2D1::RectF(62,270,W-62,355), 20, brushCard_.Get());
        DrawTextLine(L"Profile", 92, 290, 180, 30, false);
        DrawTextLine(Widen(settings_.profileName), W-320, 290, 220, 30, false, brushMuted_.Get());
        DrawRoundedCard(D2D1::RectF(62,375,W-62,460), 20, brushCard_.Get());
        DrawTextLine(L"Reduced Motion", 92, 395, 220, 30, false);
        DrawTextLine(settings_.reducedMotion ? L"On" : L"Off", W-240, 395, 100, 30, false, brushMuted_.Get());
    }

    if (!status_.empty() && page_ != Page::GameDetail) DrawTextLine(status_, 64, H-110, W-130, 38, false, brushMuted_.Get());
    DrawOverlay(W, H);
    HRESULT hr = target_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        target_.Reset();
        cachedHero_.Reset();
        cachedHeroPath_.clear();
    }
}

void App::SetOverlayVisible(bool visible) {
    if (overlayVisible_ == visible) return;
    overlayVisible_ = visible;
    overlayIndex_ = 0;
    runtime_.SetOverlayVisible(visible);
    if (visible) {
        SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        ShowWindow(hwnd_, SW_SHOW);
        SetForegroundWindow(hwnd_);
    } else {
        SetWindowPos(hwnd_, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
}

void App::Tick() {
    runtime_.Poll();
    if (runtime_.State() == RuntimeState::Crashed) {
        status_ = L"The game closed unexpectedly. ZERO recorded diagnostics and is still running.";
        if (overlayVisible_) SetOverlayVisible(false);
    } else if (runtime_.State() == RuntimeState::Exited) {
        if (status_.empty()) status_ = L"Game session ended.";
        if (overlayVisible_) SetOverlayVisible(false);
    }
    HandleInput(input_.Poll());
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::NavigateTo(Page p) {
    page_ = p;
    status_.clear();
    if (p == Page::Captures) captures_.Refresh();
}

void App::HandleInput(const InputSnapshot& in) {
    if (overlayVisible_) {
        if (in.up && overlayIndex_ > 0) --overlayIndex_;
        if (in.down && overlayIndex_ < 4) ++overlayIndex_;
        if (in.menu || in.back) SetOverlayVisible(false);
        else if (in.select) {
            if (overlayIndex_ == 0) {
                SetOverlayVisible(false);
            } else if (overlayIndex_ == 1) {
                SetOverlayVisible(false);
                navIndex_ = 3;
                NavigateTo(Page::Friends);
            } else if (overlayIndex_ == 2) {
                SetOverlayVisible(false);
                navIndex_ = 4;
                NavigateTo(Page::Captures);
            } else if (overlayIndex_ == 3) {
                const auto count = runtime_.Achievements(runtime_.Info().packageId).size();
                status_ = L"Achievements unlocked: " + std::to_wstring(count);
            } else if (overlayIndex_ == 4) {
                runtime_.Terminate();
                SetOverlayVisible(false);
                status_ = L"Game session ended.";
            }
        }
        return;
    }

    if (in.menu && runtime_.IsActive()) {
        SetOverlayVisible(true);
        return;
    }

    const auto n = registry_.Games().size();
    if (page_ == Page::GameDetail && n) {
        const auto& game = registry_.Games()[std::min(selectedGame_, n - 1)];
        const bool hasResume = game.zeroResume && resumeStore_.Load(game.packageId).has_value();
        if (hasResume && in.left) preferResume_ = false;
        if (hasResume && in.right) preferResume_ = true;
    } else {
        auto pageForNav = [](size_t index) {
            switch (index) {
                case 0: return Page::Home;
                case 1: return Page::Library;
                case 2: return Page::Store;
                case 3: return Page::Friends;
                case 4: return Page::Captures;
                default: return Page::Settings;
            }
        };
        if ((in.left || in.shoulderLeft) && navIndex_>0) {
            navIndex_--;
            NavigateTo(pageForNav(navIndex_));
        }
        if ((in.right || in.shoulderRight) && navIndex_<5) {
            navIndex_++;
            NavigateTo(pageForNav(navIndex_));
        }
    }

    if (page_ == Page::Library) {
        if (in.action) {
            NavigateTo(Page::Import);
        } else if (n) {
            if (in.up && selectedGame_>0) selectedGame_--;
            if (in.down && selectedGame_+1<n) selectedGame_++;
            if (in.select) { preferResume_ = true; NavigateTo(Page::GameDetail); }
        }
    } else if (page_ == Page::Import && in.select) {
        ImportGameFolder();
    } else if (page_ == Page::Home && in.select && n) {
        const auto& game = registry_.Games()[std::min(selectedGame_, n - 1)];
        const bool hasResume = game.zeroResume && resumeStore_.Load(game.packageId).has_value();
        LaunchSelected(hasResume);
    } else if (page_ == Page::GameDetail && in.select && n) {
        LaunchSelected(preferResume_);
    }

    if (in.back) {
        if (page_ == Page::GameDetail || page_ == Page::Import) {
            navIndex_=1;
            NavigateTo(Page::Library);
        } else if (page_ != Page::Home) {
            navIndex_=0;
            NavigateTo(Page::Home);
        }
    }
}

void App::ImportGameFolder() {
    ComPtr<IFileOpenDialog> dialog;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(dialog.GetAddressOf())))) {
        status_ = L"ZERO could not open the folder picker.";
        return;
    }
    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    dialog->SetTitle(L"Choose a ZERO-compatible game folder");
    const HRESULT shown = dialog->Show(hwnd_);
    if (shown == HRESULT_FROM_WIN32(ERROR_CANCELLED)) return;
    if (FAILED(shown)) { status_ = L"ZERO could not open the selected folder."; return; }

    ComPtr<IShellItem> item;
    if (FAILED(dialog->GetResult(item.GetAddressOf()))) { status_ = L"ZERO could not read the selected folder."; return; }
    PWSTR raw = nullptr;
    if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &raw)) || !raw) { status_ = L"ZERO could not resolve the selected folder."; return; }
    std::filesystem::path source(raw);
    CoTaskMemFree(raw);

    const auto result = importer_.ImportFolder(source);
    if (!result.success) {
        status_ = result.error.empty() ? L"ZERO rejected this game package." : result.error;
        return;
    }
    registry_.Refresh();
    selectedGame_ = registry_.Games().empty() ? 0 : registry_.Games().size() - 1;
    status_ = L"Game imported successfully.";
    navIndex_ = 1;
    page_ = Page::Library;
}

void App::LaunchSelected(bool useResume) {
    const auto& games = registry_.Games();
    if (games.empty() || selectedGame_>=games.size()) return;
    const auto& game = games[selectedGame_];
    std::optional<ResumeMetadata> resume;
    if (useResume && game.zeroResume) resume = resumeStore_.Load(game.packageId);
    std::wstring error;
    if (runtime_.Launch(game, error, resume)) {
        status_ = (resume ? L"Resuming " : L"Launching ") + Widen(game.title) + L"...";
    } else status_ = error;
}

LRESULT CALLBACK App::WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    App* self = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = reinterpret_cast<App*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }
    return self ? self->HandleMessage(msg,wp,lp) : DefWindowProcW(hwnd,msg,wp,lp);
}

LRESULT App::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch(msg) {
        case WM_PAINT: { PAINTSTRUCT ps{}; BeginPaint(hwnd_,&ps); Paint(); EndPaint(hwnd_,&ps); return 0; }
        case WM_SIZE: if (target_) target_->Resize(D2D1::SizeU(LOWORD(lp),HIWORD(lp))); return 0;
        case WM_TIMER: Tick(); return 0;
        case WM_KEYDOWN:
            if (wp == VK_F5) { registry_.Refresh(); captures_.Refresh(); cachedHero_.Reset(); cachedHeroPath_.clear(); status_=L"ZERO data refreshed."; return 0; }
            if (wp == 'I') { NavigateTo(Page::Import); return 0; }
            if (wp == VK_F11) { EnterBorderlessFullscreen(); return 0; }
            if (wp == VK_F1 && runtime_.IsActive()) { SetOverlayVisible(!overlayVisible_); return 0; }
            if (wp == 'Q' && (GetKeyState(VK_CONTROL)&0x8000)) { DestroyWindow(hwnd_); return 0; }
            return 0;
        case WM_DESTROY: runtime_.Terminate(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd_,msg,wp,lp);
}

} // namespace zero
