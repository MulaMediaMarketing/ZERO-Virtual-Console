#include "App.h"
#include <shlobj.h>
#include <shobjidl.h>
#include <filesystem>
#include <sstream>
#include <algorithm>

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
      settingsStore_(localRoot()) {}

int App::Run() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    settings_ = settingsStore_.Load();
    registry_.Refresh();
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
    if (cachedHero_) target_->DrawBitmap(cachedHero_.Get(), bounds, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    else DrawRoundedCard(bounds, 30, brushCard_.Get());
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

void App::Paint() {
    CreateDeviceResources();
    if (!target_) return;
    target_->BeginDraw();
    target_->Clear(D2D1::ColorF(0xFBFAF7));
    RECT rc{}; GetClientRect(hwnd_, &rc);
    const float W = float(rc.right), H = float(rc.bottom);

    DrawTextLine(L"ZERO", 62, 38, 240, 60, true);
    DrawTextLine(L"VIRTUAL CONSOLE", 64, 79, 280, 34, false, brushMuted_.Get());

    const wchar_t* nav[] = {L"Home", L"Library", L"Import", L"Settings"};
    for (int i=0;i<4;i++) {
        const float x = 390.0f + i*145.0f;
        if (navIndex_ == size_t(i)) DrawRoundedCard(D2D1::RectF(x-18,45,x+104,87), 18, brushCard_.Get());
        DrawTextLine(nav[i], x, 54, 110, 30, false);
    }

    const auto& games = registry_.Games();
    if (page_ == Page::Home) {
        DrawTextLine(L"Good evening, " + Widen(settings_.profileName), 62, 154, 700, 60, true);
        DrawTextLine(L"Your games. One calm console experience.", 64, 204, 650, 40, false, brushMuted_.Get());
        if (games.empty()) {
            DrawRoundedCard(D2D1::RectF(62,285,W-62,560), 28, brushCard_.Get());
            DrawTextLine(L"No games installed", 100, 330, 500, 50, true);
            DrawTextLine(L"Open Import and select a ZERO-compatible game folder.", 102, 395, W-230, 90, false, brushMuted_.Get());
        } else {
            const auto& g = games[std::min(selectedGame_, games.size()-1)];
            const auto hero = D2D1::RectF(62,285,W-62,620);
            DrawHeroArtwork(g, hero);
            ComPtr<ID2D1SolidColorBrush> shade;
            target_->CreateSolidColorBrush(D2D1::ColorF(0x000000, 0.50f), shade.GetAddressOf());
            target_->FillRectangle(hero, shade.Get());
            ComPtr<ID2D1SolidColorBrush> white;
            target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
            DrawTextLine(Widen(g.title), 108, 334, 760, 60, true, white.Get());
            DrawTextLine(L"Version " + Widen(g.version), 110, 392, 400, 35, false, white.Get());
            const auto resume = g.zeroResume ? resumeStore_.Load(g.packageId) : std::nullopt;
            DrawRoundedCard(D2D1::RectF(108,500,300,556), 20, white.Get());
            DrawTextLine(resume ? L"Resume" : L"Play", 160, 515, 120, 30, false, brushText_.Get());
            if (resume) DrawTextLine(L"Continue: " + Widen(resume->displayLabel), 330, 515, 500, 30, false, white.Get());
            DrawTextLine(L"A  Select     B  Back     F5  Refresh", 64, H-62, 700, 32, false, brushMuted_.Get());
        }
    } else if (page_ == Page::Library) {
        DrawTextLine(L"Your Library", 62, 154, 500, 60, true);
        DrawTextLine(std::to_wstring(games.size()) + L" installed", 64, 204, 250, 30, false, brushMuted_.Get());
        float y=280;
        for (size_t i=0;i<games.size() && i<8;i++,y+=78) {
            auto rect = D2D1::RectF(62,y,W-62,y+62);
            if (i==selectedGame_) DrawRoundedCard(rect, 18, brushCard_.Get());
            DrawTextLine(Widen(games[i].title), 92, y+14, 540, 30, false);
            DrawTextLine(Widen(games[i].version), W-250, y+14, 120, 30, false, brushMuted_.Get());
        }
    } else if (page_ == Page::GameDetail) {
        if (!games.empty()) {
            const auto& g = games[std::min(selectedGame_, games.size()-1)];
            DrawTextLine(Widen(g.title), 62, 154, 700, 60, true);
            DrawTextLine(Widen(g.packageId), 64, 208, 700, 35, false, brushMuted_.Get());
            const auto platform = runtime_.PlatformState(g.packageId);
            DrawTextLine(L"Playtime  " + std::to_wstring(platform.totalPlaytimeSeconds / 60) + L" min", 64, 280, 400, 30, false);
            DrawTextLine(L"Launches  " + std::to_wstring(platform.launchCount), 64, 320, 400, 30, false);
            const auto resume = g.zeroResume ? resumeStore_.Load(g.packageId) : std::nullopt;
            DrawRoundedCard(D2D1::RectF(64,410,264,468), 20, brushAccent_.Get());
            ComPtr<ID2D1SolidColorBrush> white; target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
            DrawTextLine(resume ? L"Resume" : L"Play", 120, 426, 110, 30, false, white.Get());
            if (resume) DrawTextLine(L"Resume point: " + Widen(resume->displayLabel), 64, 500, W-130, 40, false, brushMuted_.Get());
            DrawTextLine(status_, 64, 560, W-130, 60, false, brushMuted_.Get());
        }
    } else if (page_ == Page::Import) {
        DrawTextLine(L"Import a game", 62, 154, 600, 60, true);
        DrawTextLine(L"Add a folder that contains a valid zero.manifest.json and native Windows game executable.",
            64, 214, W-128, 48, false, brushMuted_.Get());
        DrawRoundedCard(D2D1::RectF(62,310,420,390), 22, brushAccent_.Get());
        ComPtr<ID2D1SolidColorBrush> white; target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
        DrawTextLine(L"A   Choose game folder", 100, 334, 280, 32, false, white.Get());
        DrawTextLine(L"ZERO validates the package, stages it safely, and adds it to your Library.",
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
    HRESULT hr = target_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        target_.Reset();
        cachedHero_.Reset();
        cachedHeroPath_.clear();
    }
}

void App::Tick() {
    runtime_.Poll();
    if (runtime_.State() == RuntimeState::Crashed) status_ = L"The game closed unexpectedly. ZERO recorded diagnostics and is still running.";
    else if (runtime_.State() == RuntimeState::Exited && status_.empty()) status_ = L"Game session ended.";
    HandleInput(input_.Poll());
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::NavigateTo(Page p) { page_ = p; status_.clear(); }

void App::HandleInput(const InputSnapshot& in) {
    if (in.left && navIndex_>0) {
        navIndex_--;
        NavigateTo(navIndex_==0?Page::Home:navIndex_==1?Page::Library:navIndex_==2?Page::Import:Page::Settings);
    }
    if (in.right && navIndex_<3) {
        navIndex_++;
        NavigateTo(navIndex_==0?Page::Home:navIndex_==1?Page::Library:navIndex_==2?Page::Import:Page::Settings);
    }
    const auto n = registry_.Games().size();
    if (page_ == Page::Library && n) {
        if (in.up && selectedGame_>0) selectedGame_--;
        if (in.down && selectedGame_+1<n) selectedGame_++;
        if (in.select) NavigateTo(Page::GameDetail);
    } else if (page_ == Page::Import && in.select) {
        ImportGameFolder();
    } else if ((page_ == Page::Home || page_ == Page::GameDetail) && in.select && n) {
        LaunchSelected();
    }
    if (in.back) {
        if (page_ == Page::GameDetail) NavigateTo(Page::Library);
        else if (page_ != Page::Home) { navIndex_=0; NavigateTo(Page::Home); }
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

void App::LaunchSelected() {
    const auto& games = registry_.Games();
    if (games.empty() || selectedGame_>=games.size()) return;
    const auto& game = games[selectedGame_];
    const auto resume = game.zeroResume ? resumeStore_.Load(game.packageId) : std::nullopt;
    std::wstring error;
    if (runtime_.Launch(game, error)) {
        status_ = (resume ? L"Launching resume activity for " : L"Launching ") + Widen(game.title) + L"...";
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
            if (wp == VK_F5) { registry_.Refresh(); cachedHero_.Reset(); cachedHeroPath_.clear(); status_=L"Library refreshed."; return 0; }
            if (wp == 'I') { navIndex_=2; NavigateTo(Page::Import); return 0; }
            if (wp == VK_F11) { EnterBorderlessFullscreen(); return 0; }
            if (wp == 'Q' && (GetKeyState(VK_CONTROL)&0x8000)) { DestroyWindow(hwnd_); return 0; }
            return 0;
        case WM_DESTROY: runtime_.Terminate(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd_,msg,wp,lp);
}
}
