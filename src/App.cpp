#include "App.h"
#include <shobjidl.h>
#include <algorithm>
#include <filesystem>

using Microsoft::WRL::ComPtr;

namespace zero {

App::App(HINSTANCE instance, ApplicationServices& services)
    : instance_(instance),
      services_(services),
      registry_(services.Registry()),
      importer_(services.Importer()),
      downloads_(services.Downloads()),
      captures_(services.Captures()),
      identity_(services.Identity()),
      friends_(services.Friends()),
      store_(services.Store()),
      runtime_(services.Runtime()),
      settingsStore_(services.Settings()),
      input_(services.PlayerInput()),
      productionShell_(services.Shell()),
      resumeStore_(runtime_),
      page_(productionShell_),
      navIndex_(*this, productionShell_) {}

int App::Run() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    settings_ = settingsStore_.Load();
    settings_.volume = ClampVolume(settings_.volume);
    shellUx_.SetReducedMotion(settings_.reducedMotion);
    lastTick_ = std::chrono::steady_clock::now();

    std::wstring identityError;
    if (!identity_.Initialize(settings_.profileName, identityError)) status_ = identityError;
    friends_.Refresh();
    store_.Refresh();
    registry_.Refresh();
    captures_.Refresh();

    ClampFriendsExperience(friendsUx_, friends_.State(), friends_.Friends());
    ClampStoreSelection(storeUx_, store_.State(), store_.Products().size());
    ClampCoreShellSelection(coreShellUx_, registry_.Games().size());
    ClampCaptureExperience(capturesUx_, captures_.Items());
    ClampSettingsSelection(settingsUx_);

    if (!InitWindow() || !InitGraphics()) return 1;
    ShowWindow(hwnd_, SW_MAXIMIZE);
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
    hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, L"ZERO Player",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT, CW_USEDEFAULT, 1600, 900,
        nullptr, nullptr, instance_, this);
    return hwnd_ != nullptr;
}

void App::EnterBorderlessFullscreen() {
    if (!hwnd_) return;
    ShowWindow(hwnd_, IsZoomed(hwnd_) ? SW_RESTORE : SW_MAXIMIZE);
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
    CancelCaptureModal(capturesUx_, captures_.Items());

    if (next == Page::Captures) {
        captures_.Refresh();
        ClampCaptureExperience(capturesUx_, captures_.Items());
    } else if (next == Page::Friends) {
        friends_.Refresh();
        ClampFriendsExperience(friendsUx_, friends_.State(), friends_.Friends());
    } else if (next == Page::Store) {
        store_.Refresh();
        ClampStoreSelection(storeUx_, store_.State(), store_.Products().size());
    } else if (next == Page::Achievements) {
        ClampAchievementSelection();
    } else if (next == Page::Settings) {
        ClampSettingsSelection(settingsUx_);
        RefreshSettingsTelemetry();
    } else if (next == Page::Home || next == Page::Library || next == Page::GameDetail) {
        ClampCoreShellSelection(coreShellUx_, registry_.Games().size());
    }
}

void App::HandleCaptureInput(const InputSnapshot& in) {
    const auto& items = captures_.Items();
    ClampCaptureExperience(capturesUx_, items);

    if (capturesUx_.mode == CaptureExperienceMode::DeleteConfirm) {
        if (in.back) {
            CancelCaptureModal(capturesUx_, items);
            NotifyFocusMoved();
            return;
        }
        const auto selected = SelectedCaptureIndex(capturesUx_, items);
        if (in.select && selected) {
            std::wstring error;
            const auto item = items[*selected];
            if (captures_.Delete(item, error)) {
                status_ = L"Capture deleted.";
                cachedCapture_.Reset();
                cachedCapturePath_.clear();
                OnCaptureDeleted(capturesUx_, captures_.Items());
                NotifyFocusMoved();
            } else {
                status_ = error;
                CancelCaptureModal(capturesUx_, captures_.Items());
            }
        }
        return;
    }

    if (capturesUx_.mode == CaptureExperienceMode::Viewer) {
        if (in.back) {
            CancelCaptureModal(capturesUx_, items);
            NotifyFocusMoved();
            return;
        }
        const auto selected = SelectedCaptureIndex(capturesUx_, items);
        if (!selected) {
            CancelCaptureModal(capturesUx_, items);
            return;
        }
        if (in.action) {
            if (BeginCaptureDelete(capturesUx_, items)) NotifyFocusMoved();
            return;
        }
        if (in.right) {
            std::wstring error;
            if (!captures_.Reveal(items[*selected], error)) status_ = error;
        }
        return;
    }

    if (items.empty()) return;
    if (in.up && MoveCaptureSelectionUp(capturesUx_, items)) NotifyFocusMoved();
    if (in.down && MoveCaptureSelectionDown(capturesUx_, items)) NotifyFocusMoved();
    const auto selected = SelectedCaptureIndex(capturesUx_, items);
    if (!selected) return;
    if (in.select) {
        const auto& item = items[*selected];
        if (CaptureCanViewInline(item)) {
            if (OpenCaptureViewer(capturesUx_, items)) {
                cachedCapture_.Reset();
                cachedCapturePath_.clear();
                NotifyFocusMoved();
            }
        } else if (CaptureCanOpenExternally(item)) {
            std::wstring error;
            if (!captures_.Open(item, error)) status_ = error;
        }
    }
    if (in.action && BeginCaptureDelete(capturesUx_, items)) NotifyFocusMoved();
    if (in.right) {
        std::wstring error;
        if (!captures_.Reveal(items[*selected], error)) status_ = error;
    }
}

void App::HandleInput(const InputSnapshot& in) {
    if (launchUxMode_ != LaunchUxMode::Hidden) {
        HandleLaunchRecoveryInput(in);
        return;
    }
    if (capturesUx_.mode == CaptureExperienceMode::DeleteConfirm || capturesUx_.mode == CaptureExperienceMode::Viewer) {
        HandleCaptureInput(in);
        return;
    }
    if (overlayVisible_) {
        if (achievementsFromOverlay_) {
            HandleAchievementInput(in);
            return;
        }
        if (in.up && overlayIndex_ > 0) { --overlayIndex_; NotifyFocusMoved(); }
        if (in.down && overlayIndex_ < 4) { ++overlayIndex_; NotifyFocusMoved(); }
        if (in.menu || in.back) SetOverlayVisible(false);
        else if (in.select) {
            if (overlayIndex_ == 0) SetOverlayVisible(false);
            else if (overlayIndex_ == 1) { SetOverlayVisible(false); navIndex_ = ProductionUxIndex(ProductionUxDestination::Friends); NavigateTo(Page::Friends); }
            else if (overlayIndex_ == 2) { SetOverlayVisible(false); navIndex_ = ProductionUxIndex(ProductionUxDestination::Captures); NavigateTo(Page::Captures); }
            else if (overlayIndex_ == 3) OpenAchievements(runtime_.Info().packageId, page_, true);
            else { runtime_.Terminate(); SetOverlayVisible(false); }
        }
        return;
    }
    if (in.menu && runtime_.IsActive()) { SetOverlayVisible(true); return; }

    const bool topLevel = ProductionUxIsTopLevelPage(page_);
    if (topLevel && in.left) navIndex_ = navIndex_ > 0 ? navIndex_ - 1 : ProductionUxNavCount() - 1;
    if (topLevel && in.right) navIndex_ = (navIndex_ + 1) % ProductionUxNavCount();
    if (topLevel && (in.left || in.right)) NavigateTo(ProductionUxPageAt(navIndex_));

    if (page_ == Page::Captures) { HandleCaptureInput(in); return; }
    if (page_ == Page::Achievements) { HandleAchievementInput(in); return; }
    if (page_ == Page::Settings) { HandleSettingsInput(in); return; }
    if (page_ == Page::Home || page_ == Page::Library || page_ == Page::GameDetail || page_ == Page::Import) {
        HandleCoreShellInput(in);
        return;
    }
}

} // namespace zero
