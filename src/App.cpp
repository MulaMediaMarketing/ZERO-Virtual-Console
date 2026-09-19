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
    services_.ServiceState().RefreshAll();

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

    services_.ServiceState().PollRuntime();
    UpdateLaunchUx();
    if (!overlayClosing_) HandleInput(input_.Poll());
    else input_.Poll();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::NavigateTo(Page next) {
    const Page current = page_;
    if (current != next) {
        const auto beforeRevision = productionShell_.Snapshot().navigationRevision;
        std::string error;
        if (!productionShell_.Navigate(next, error)) {
            status_ = Widen(error.empty() ? "ZERO could not navigate to the requested page." : error);
            return;
        }
        const auto active = productionShell_.ActivePage();
        if (!active || *active != next) {
            status_ = L"ZERO shell navigation did not commit the requested page.";
            return;
        }
        if (productionShell_.Snapshot().navigationRevision != beforeRevision) {
            shellUx_.BeginPageTransition();
            NotifyFocusMoved();
        }
    }
    status_.clear();
    CancelCaptureModal(capturesUx_, captures_.Items());
    services_.ServiceState().RefreshForPage(next);

    if (next == Page::Captures) {
        ClampCaptureExperience(capturesUx_, captures_.Items());
    } else if (next == Page::Friends) {
        ClampFriendsExperience(friendsUx_, friends_.State(), friends_.Friends());
    } else if (next == Page::Store) {
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

    const auto globalIntent = services_.ShellCoordinator().RouteGlobalInput(in, runtime_.IsActive(), overlayVisible_);
    if (globalIntent.type == shell::ShellIntentType::ToggleOverlay) {
        SetOverlayVisible(!overlayVisible_ || overlayClosing_);
        return;
    }
    if (globalIntent.type == shell::ShellIntentType::MoveTopLevel && page_ != Page::GameDetail) {
        ProductionUxPage next = page_;
        std::string error;
        if (services_.ShellCoordinator().MoveTopLevel(globalIntent.direction, next, error)) NavigateTo(next);
        else status_ = Widen(error);
        return;
    }

    if (page_ == Page::Achievements) { HandleAchievementInput(in); return; }

    if (page_ == Page::Captures && !(in.shoulderLeft || in.shoulderRight)) {
        HandleCaptureInput(in);
        if (in.up || in.down || in.select || in.action || in.right) return;
    }
    if (page_ == Page::Friends && !(in.shoulderLeft || in.shoulderRight)) {
        const auto& list = friends_.Friends();
        if (in.up && MoveFriendSelectionUp(friendsUx_, friends_.State(), list)) NotifyFocusMoved();
        if (in.down && MoveFriendSelectionDown(friendsUx_, friends_.State(), list)) NotifyFocusMoved();
        if (in.select) {
            const auto* selected = SelectedFriend(friendsUx_, friends_.State(), list);
            if (selected && FriendCanJoin(*selected)) status_ = L"Joinable presence is available, but the online join transport is not connected in this build.";
        }
        if (in.up || in.down || in.select) return;
    }
    if (page_ == Page::Store && !(in.shoulderLeft || in.shoulderRight)) {
        const auto count = store_.Products().size();
        if (in.up && MoveStoreSelectionUp(storeUx_, store_.State(), count)) NotifyFocusMoved();
        if (in.down && MoveStoreSelectionDown(storeUx_, store_.State(), count)) NotifyFocusMoved();
        if (in.select && storeUx_.mode == StoreExperienceMode::Browsing && storeUx_.selectedIndex < count) {
            const auto& product = store_.Products()[storeUx_.selectedIndex];
            if (StoreProductIsOwned(product)) status_ = L"This product is already owned.";
            else if (StoreProductCanLaunchPurchase(product)) status_ = L"Store purchase transport is not connected in this build.";
            else status_ = L"This product does not expose a valid purchase action.";
        }
        if (in.up || in.down || in.select) return;
    }
    if (page_ == Page::Settings && !(in.shoulderLeft || in.shoulderRight)) {
        const bool volumeHorizontal = settingsUx_.selectedRow == static_cast<size_t>(SettingsExperienceRow::Volume) && (in.left || in.right);
        HandleSettingsInput(in);
        if (in.up || in.down || in.select || in.action || volumeHorizontal) return;
    }

    const auto gameCount = registry_.Games().size();
    ClampCoreShellSelection(coreShellUx_, gameCount);
    if (page_ == Page::GameDetail && gameCount) {
        const auto& game = registry_.Games()[coreShellUx_.selectedGame];
        const bool hasResume = game.zeroResume && resumeStore_.Load(game.packageId).has_value();
        if (hasResume && in.left && preferResume_) { preferResume_ = false; NotifyFocusMoved(); }
        if (hasResume && in.right && !preferResume_) { preferResume_ = true; NotifyFocusMoved(); }
        if (in.action) { OpenAchievements(game.packageId, Page::GameDetail, false); return; }
    } else {
        if (in.left) {
            ProductionUxPage next = page_;
            std::string error;
            if (services_.ShellCoordinator().MoveTopLevel(-1, next, error)) NavigateTo(next);
            else status_ = Widen(error);
        }
        if (in.right) {
            ProductionUxPage next = page_;
            std::string error;
            if (services_.ShellCoordinator().MoveTopLevel(1, next, error)) NavigateTo(next);
            else status_ = Widen(error);
        }
    }

    if (page_ == Page::Library) {
        if (in.action) NavigateTo(Page::Import);
        else if (gameCount) {
            if (in.up && MoveGameSelectionUp(coreShellUx_, gameCount)) NotifyFocusMoved();
            if (in.down && MoveGameSelectionDown(coreShellUx_, gameCount)) NotifyFocusMoved();
            if (in.select) { preferResume_ = true; NavigateTo(Page::GameDetail); }
        }
    } else if (page_ == Page::Import && in.select) ImportGameFolder();
    else if (page_ == Page::Home && in.select && gameCount) {
        const auto& game = registry_.Games()[coreShellUx_.selectedGame];
        const bool hasResume = game.zeroResume && resumeStore_.Load(game.packageId).has_value();
        LaunchSelected(hasResume);
    } else if (page_ == Page::GameDetail && in.select && gameCount) LaunchSelected(preferResume_);

    if (globalIntent.type == shell::ShellIntentType::Back) {
        ProductionUxPage next = page_;
        std::string error;
        if (services_.ShellCoordinator().Back(next, error)) NavigateTo(next);
        else status_ = Widen(error.empty() ? "ZERO could not navigate back." : error);
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
    if (!result.success) { status_ = result.error.empty() ? L"ZERO rejected this game package." : result.error; return; }

    registry_.Refresh();
    coreShellUx_.selectedGame = registry_.Games().empty() ? 0 : registry_.Games().size() - 1;
    ClampCoreShellSelection(coreShellUx_, registry_.Games().size());
    navIndex_ = ProductionUxIndex(ProductionUxDestination::Library);
    NavigateTo(Page::Library);
    status_ = L"Game imported successfully.";
}

void App::LaunchSelected(bool useResume) {
    const auto& games = registry_.Games();
    ClampCoreShellSelection(coreShellUx_, games.size());
    if (games.empty() || coreShellUx_.selectedGame >= games.size()) return;
    const auto& game = games[coreShellUx_.selectedGame];

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
    if (runtime_.Launch(game, error, resume)) { status_.clear(); return; }

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
        case WM_GETMINMAXINFO: {
            auto* info = reinterpret_cast<MINMAXINFO*>(lp);
            if (info) {
                info->ptMinTrackSize.x = 1280;
                info->ptMinTrackSize.y = 720;
            }
            return 0;
        }
        case WM_LBUTTONDOWN: {
            if (launchUxMode_ != LaunchUxMode::Hidden || overlayVisible_) return 0;
            const int x = GET_X_LPARAM(lp);
            const int y = GET_Y_LPARAM(lp);
            if (x >= 8 && x <= 214 && y >= 126) {
                const int relative = y - 126;
                const size_t index = static_cast<size_t>(relative / 52);
                const int within = relative % 52;
                if (index < ProductionUxNavCount() && within <= 42) {
                    navIndex_ = index;
                    NavigateTo(ProductionUxPageAt(index));
                    return 0;
                }
            }
            return 0;
        }
        case WM_TIMER:
            Tick();
            return 0;
        case WM_KEYDOWN:
            if (wp == VK_F5) {
                services_.ServiceState().RefreshAll();
                ClampCoreShellSelection(coreShellUx_, registry_.Games().size());
                ClampCaptureExperience(capturesUx_, captures_.Items());
                ClampFriendsExperience(friendsUx_, friends_.State(), friends_.Friends());
                ClampStoreSelection(storeUx_, store_.State(), store_.Products().size());
                ClampSettingsSelection(settingsUx_); ClampAchievementSelection(); RefreshSettingsTelemetry();
                cachedHero_.Reset(); cachedHeroPath_.clear(); cachedCapture_.Reset(); cachedCapturePath_.clear();
                status_ = L"ZERO data refreshed.";
                return 0;
            }
            if (wp == 'I') { NavigateTo(Page::Import); return 0; }
            if (wp == VK_F11) { EnterBorderlessFullscreen(); return 0; }
            if (wp == VK_F1 && runtime_.IsActive()) { SetOverlayVisible(!overlayVisible_ || overlayClosing_); return 0; }
            if (wp == 'Q' && (GetKeyState(VK_CONTROL) & 0x8000)) { DestroyWindow(hwnd_); return 0; }
            return 0;
        case WM_DESTROY:
            runtime_.Terminate();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd_, msg, wp, lp);
}

} // namespace zero
