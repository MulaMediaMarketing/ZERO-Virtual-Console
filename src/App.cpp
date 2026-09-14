#include "App.h"
#include <shlobj.h>
#include <shobjidl.h>
#include <algorithm>
#include <filesystem>

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

App::Page pageForNavIndex(size_t index) {
    switch (index) {
        case 0: return App::Page::Home;
        case 1: return App::Page::Library;
        case 2: return App::Page::Store;
        case 3: return App::Page::Friends;
        case 4: return App::Page::Captures;
        default: return App::Page::Settings;
    }
}
}

App::App(HINSTANCE instance)
    : instance_(instance),
      registry_(localRoot() / "Library"),
      importer_(localRoot() / "Library"),
      captures_(localRoot() / "Captures"),
      identity_(localRoot()),
      settingsStore_(localRoot()) {}

int App::Run() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    settings_ = settingsStore_.Load();
    shellUx_.SetReducedMotion(settings_.reducedMotion);
    lastTick_ = std::chrono::steady_clock::now();

    std::wstring identityError;
    if (!identity_.Initialize(settings_.profileName, identityError)) status_ = identityError;
    friends_.Refresh();
    store_.Refresh();
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
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(wicFactory_.GetAddressOf())))) return false;

    if (FAILED(writeFactory_->CreateTextFormat(L"Segoe UI Variable Display", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 34.0f, L"en-us", heading_.GetAddressOf()))) return false;
    if (FAILED(writeFactory_->CreateTextFormat(L"Segoe UI Variable Text", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 18.0f, L"en-us", body_.GetAddressOf()))) return false;
    heading_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    body_->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
    return true;
}

void App::CreateDeviceResources() {
    if (target_) return;
    RECT rc{};
    GetClientRect(hwnd_, &rc);
    const auto size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
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

void App::DrawTextLine(const std::wstring& text, float x, float y, float w, float h, bool head, ID2D1Brush* brush) {
    if (!target_) return;
    target_->DrawTextW(text.c_str(), static_cast<UINT32>(text.size()), head ? heading_.Get() : body_.Get(),
        D2D1::RectF(x, y, x + w, y + h), brush ? brush : brushText_.Get());
}

void App::DrawRoundedCard(const D2D1_RECT_F& rect, float radius, ID2D1Brush* fill) {
    if (!target_ || !fill) return;
    target_->FillRoundedRectangle(D2D1::RoundedRect(rect, radius, radius), fill);
}

std::wstring App::Widen(const std::string& text) const {
    if (text.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (count <= 0) return std::wstring(text.begin(), text.end());
    std::wstring out(static_cast<size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), count);
    return out;
}

void App::ClampCaptureSelection() {
    const auto count = captures_.Items().size();
    if (count == 0) {
        selectedCapture_ = 0;
        captureScroll_ = 0;
        return;
    }
    if (selectedCapture_ >= count) selectedCapture_ = count - 1;
    constexpr size_t visible = 6;
    if (selectedCapture_ < captureScroll_) captureScroll_ = selectedCapture_;
    if (selectedCapture_ >= captureScroll_ + visible) captureScroll_ = selectedCapture_ - visible + 1;
    const size_t maxStart = count > visible ? count - visible : 0;
    captureScroll_ = std::min(captureScroll_, maxStart);
}

void App::SetOverlayVisible(bool visible) {
    if (visible) {
        if (overlayVisible_ && !overlayClosing_) return;
        overlayVisible_ = true;
        overlayClosing_ = false;
        overlayIndex_ = 0;
        shellUx_.SetOverlayVisible(true);
        NotifyFocusMoved();
        runtime_.SetOverlayVisible(true);
        SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        ShowWindow(hwnd_, SW_SHOW);
        SetForegroundWindow(hwnd_);
        return;
    }

    if (!overlayVisible_ || overlayClosing_) return;
    overlayClosing_ = true;
    shellUx_.SetOverlayVisible(false);
    runtime_.SetOverlayVisible(false);
    NotifyFocusMoved();

    if (shellUx_.ReducedMotion()) {
        overlayClosing_ = false;
        overlayVisible_ = false;
        achievementsFromOverlay_ = false;
        SetWindowPos(hwnd_, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
}

void App::Tick() {
    const auto now = std::chrono::steady_clock::now();
    const auto delta = now - lastTick_;
    lastTick_ = now;
    shellUx_.Advance(std::chrono::duration<float>(delta));

    if (overlayClosing_ && !shellUx_.OverlayRenderActive()) {
        overlayClosing_ = false;
        overlayVisible_ = false;
        achievementsFromOverlay_ = false;
        SetWindowPos(hwnd_, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    runtime_.Poll();
    UpdateLaunchUx();
    if (!overlayClosing_) HandleInput(input_.Poll());
    else input_.Poll();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::NavigateTo(Page next) {
    if (page_ != next) {
        page_ = next;
        shellUx_.BeginPageTransition();
        NotifyFocusMoved();
    }
    status_.clear();
    captureViewerVisible_ = false;
    captureDeleteConfirm_ = false;

    if (next == Page::Captures) {
        captures_.Refresh();
        ClampCaptureSelection();
    } else if (next == Page::Friends) {
        friends_.Refresh();
    } else if (next == Page::Store) {
        store_.Refresh();
    } else if (next == Page::Achievements) {
        ClampAchievementSelection();
    } else if (next == Page::Settings) {
        RefreshSettingsTelemetry();
    }
}

void App::HandleCaptureInput(const InputSnapshot& in) {
    const auto& items = captures_.Items();

    if (captureDeleteConfirm_) {
        if (in.back) {
            captureDeleteConfirm_ = false;
            NotifyFocusMoved();
            return;
        }
        if (in.select && !items.empty() && selectedCapture_ < items.size()) {
            std::wstring error;
            const auto item = items[selectedCapture_];
            if (captures_.Delete(item, error)) {
                status_ = L"Capture deleted.";
                captureDeleteConfirm_ = false;
                captureViewerVisible_ = false;
                cachedCapture_.Reset();
                cachedCapturePath_.clear();
                ClampCaptureSelection();
                NotifyFocusMoved();
            } else {
                status_ = error;
                captureDeleteConfirm_ = false;
            }
        }
        return;
    }

    if (captureViewerVisible_) {
        if (in.back) {
            captureViewerVisible_ = false;
            NotifyFocusMoved();
            return;
        }
        if (items.empty() || selectedCapture_ >= items.size()) {
            captureViewerVisible_ = false;
            return;
        }
        if (in.action) {
            captureDeleteConfirm_ = true;
            NotifyFocusMoved();
            return;
        }
        if (in.right) {
            std::wstring error;
            if (!captures_.Reveal(items[selectedCapture_], error)) status_ = error;
            return;
        }
        return;
    }

    if (items.empty()) return;
    if (in.up && selectedCapture_ > 0) {
        --selectedCapture_;
        ClampCaptureSelection();
        NotifyFocusMoved();
    }
    if (in.down && selectedCapture_ + 1 < items.size()) {
        ++selectedCapture_;
        ClampCaptureSelection();
        NotifyFocusMoved();
    }
    if (in.select && selectedCapture_ < items.size()) {
        const auto& item = items[selectedCapture_];
        if (item.kind == CaptureKind::Screenshot) {
            captureViewerVisible_ = true;
            cachedCapture_.Reset();
            cachedCapturePath_.clear();
            NotifyFocusMoved();
        } else {
            std::wstring error;
            if (!captures_.Open(item, error)) status_ = error;
        }
    }
    if (in.action && selectedCapture_ < items.size()) {
        captureDeleteConfirm_ = true;
        NotifyFocusMoved();
    }
    if (in.right && selectedCapture_ < items.size()) {
        std::wstring error;
        if (!captures_.Reveal(items[selectedCapture_], error)) status_ = error;
    }
}

void App::HandleInput(const InputSnapshot& in) {
    if (launchUxMode_ != LaunchUxMode::Hidden) {
        HandleLaunchRecoveryInput(in);
        return;
    }

    if (captureDeleteConfirm_ || captureViewerVisible_) {
        HandleCaptureInput(in);
        return;
    }

    if (overlayVisible_) {
        if (achievementsFromOverlay_) {
            HandleAchievementInput(in);
            return;
        }
        if (in.up && overlayIndex_ > 0) {
            --overlayIndex_;
            NotifyFocusMoved();
        }
        if (in.down && overlayIndex_ < 4) {
            ++overlayIndex_;
            NotifyFocusMoved();
        }
        if (in.menu || in.back) {
            SetOverlayVisible(false);
        } else if (in.select) {
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
                OpenAchievements(runtime_.Info().packageId, page_, true);
            } else {
                runtime_.Terminate();
                SetOverlayVisible(false);
            }
        }
        return;
    }

    if (in.menu && runtime_.IsActive()) {
        SetOverlayVisible(true);
        return;
    }

    if (page_ == Page::Achievements) {
        HandleAchievementInput(in);
        return;
    }

    if (page_ == Page::Captures && !(in.shoulderLeft || in.shoulderRight)) {
        HandleCaptureInput(in);
        if (in.up || in.down || in.select || in.action || in.right) return;
    }

    if (page_ == Page::Settings && !(in.shoulderLeft || in.shoulderRight)) {
        const bool volumeHorizontal = selectedSetting_ == 1 && (in.left || in.right);
        HandleSettingsInput(in);
        if (in.up || in.down || in.select || in.action || volumeHorizontal) return;
    }

    const auto gameCount = registry_.Games().size();
    if (page_ == Page::GameDetail && gameCount) {
        const auto& game = registry_.Games()[std::min(selectedGame_, gameCount - 1)];
        const bool hasResume = game.zeroResume && resumeStore_.Load(game.packageId).has_value();
        if (hasResume && in.left && preferResume_) {
            preferResume_ = false;
            NotifyFocusMoved();
        }
        if (hasResume && in.right && !preferResume_) {
            preferResume_ = true;
            NotifyFocusMoved();
        }
        if (in.action) {
            OpenAchievements(game.packageId, Page::GameDetail, false);
            return;
        }
    } else {
        if ((in.left || in.shoulderLeft) && navIndex_ > 0) {
            --navIndex_;
            NavigateTo(pageForNavIndex(navIndex_));
        }
        if ((in.right || in.shoulderRight) && navIndex_ < 5) {
            ++navIndex_;
            NavigateTo(pageForNavIndex(navIndex_));
        }
    }

    if (page_ == Page::Library) {
        if (in.action) {
            NavigateTo(Page::Import);
        } else if (gameCount) {
            if (in.up && selectedGame_ > 0) {
                --selectedGame_;
                NotifyFocusMoved();
            }
            if (in.down && selectedGame_ + 1 < gameCount) {
                ++selectedGame_;
                NotifyFocusMoved();
            }
            if (in.select) {
                preferResume_ = true;
                NavigateTo(Page::GameDetail);
            }
        }
    } else if (page_ == Page::Import && in.select) {
        ImportGameFolder();
    } else if (page_ == Page::Home && in.select && gameCount) {
        const auto& game = registry_.Games()[std::min(selectedGame_, gameCount - 1)];
        const bool hasResume = game.zeroResume && resumeStore_.Load(game.packageId).has_value();
        LaunchSelected(hasResume);
    } else if (page_ == Page::GameDetail && in.select && gameCount) {
        LaunchSelected(preferResume_);
    }

    if (in.back) {
        if (page_ == Page::GameDetail || page_ == Page::Import) {
            navIndex_ = 1;
            NavigateTo(Page::Library);
        } else if (page_ != Page::Home) {
            navIndex_ = 0;
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
    if (FAILED(shown)) {
        status_ = L"ZERO could not open the selected folder.";
        return;
    }

    ComPtr<IShellItem> item;
    if (FAILED(dialog->GetResult(item.GetAddressOf()))) {
        status_ = L"ZERO could not read the selected folder.";
        return;
    }
    PWSTR raw = nullptr;
    if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &raw)) || !raw) {
        status_ = L"ZERO could not resolve the selected folder.";
        return;
    }
    std::filesystem::path source(raw);
    CoTaskMemFree(raw);

    const auto result = importer_.ImportFolder(source);
    if (!result.success) {
        status_ = result.error.empty() ? L"ZERO rejected this game package." : result.error;
        return;
    }

    registry_.Refresh();
    selectedGame_ = registry_.Games().empty() ? 0 : registry_.Games().size() - 1;
    navIndex_ = 1;
    NavigateTo(Page::Library);
    status_ = L"Game imported successfully.";
}

void App::LaunchSelected(bool useResume) {
    const auto& games = registry_.Games();
    if (games.empty() || selectedGame_ >= games.size()) return;
    const auto& game = games[selectedGame_];

    const auto trust = runtime_.PackageTrust(game);
    if (!trust.launchAllowed) {
        status_ = trust.detail.empty() ? L"ZERO blocked launch because package trust validation failed." : trust.detail;
        launchError_.clear();
        launchUxMode_ = LaunchUxMode::Hidden;
        NotifyFocusMoved();
        return;
    }

    launchPackageId_ = game.packageId;
    launchTitle_ = Widen(game.title);
    launchUsedResume_ = false;
    launchError_.clear();
    lastRuntimeOutcome_ = RuntimeOutcome::None;

    std::optional<ResumeMetadata> resume;
    if (useResume && game.zeroResume) resume = resumeStore_.Load(game.packageId);
    launchUsedResume_ = resume.has_value();
    launchUxMode_ = LaunchUxMode::Starting;
    NotifyFocusMoved();

    std::wstring error;
    if (runtime_.Launch(game, error, resume)) {
        status_.clear();
        return;
    }

    launchError_ = error.empty() ? L"ZERO could not create the game session." : error;
    lastRuntimeOutcome_ = runtime_.Outcome();
    launchUxMode_ = LaunchUxMode::Failed;
    NotifyFocusMoved();
}

LRESULT CALLBACK App::WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    App* self = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = reinterpret_cast<App*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd_ = hwnd;
    }
    return self ? self->HandleMessage(msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT App::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            BeginPaint(hwnd_, &ps);
            Paint();
            EndPaint(hwnd_, &ps);
            return 0;
        }
        case WM_SIZE:
            if (target_) target_->Resize(D2D1::SizeU(LOWORD(lp), HIWORD(lp)));
            return 0;
        case WM_TIMER:
            Tick();
            return 0;
        case WM_KEYDOWN:
            if (wp == VK_F5) {
                registry_.Refresh();
                captures_.Refresh();
                friends_.Refresh();
                store_.Refresh();
                ClampCaptureSelection();
                ClampAchievementSelection();
                RefreshSettingsTelemetry();
                cachedHero_.Reset();
                cachedHeroPath_.clear();
                cachedCapture_.Reset();
                cachedCapturePath_.clear();
                status_ = L"ZERO data refreshed.";
                return 0;
            }
            if (wp == 'I') {
                NavigateTo(Page::Import);
                return 0;
            }
            if (wp == VK_F11) {
                EnterBorderlessFullscreen();
                return 0;
            }
            if (wp == VK_F1 && runtime_.IsActive()) {
                SetOverlayVisible(!overlayVisible_ || overlayClosing_);
                return 0;
            }
            if (wp == 'Q' && (GetKeyState(VK_CONTROL) & 0x8000)) {
                DestroyWindow(hwnd_);
                return 0;
            }
            return 0;
        case WM_DESTROY:
            runtime_.Terminate();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd_, msg, wp, lp);
}

} // namespace zero
