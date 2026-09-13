#include "App.h"
#include <algorithm>

using Microsoft::WRL::ComPtr;

namespace zero {
namespace {
std::wstring OutcomeTitle(RuntimeOutcome outcome) {
    switch (outcome) {
        case RuntimeOutcome::LaunchFailure: return L"Game could not start";
        case RuntimeOutcome::HandshakeFailure: return L"Game connection failed";
        case RuntimeOutcome::ReadyTimeout: return L"Game took too long to start";
        case RuntimeOutcome::Hung: return L"Game stopped responding";
        case RuntimeOutcome::Crash: return L"Game closed unexpectedly";
        case RuntimeOutcome::UserTermination: return L"Game session ended";
        case RuntimeOutcome::CleanExit: return L"Welcome back";
        default: return L"Returning to ZERO";
    }
}

std::wstring OutcomeBody(RuntimeOutcome outcome) {
    switch (outcome) {
        case RuntimeOutcome::LaunchFailure:
            return L"ZERO could not create the game session. The console stayed running and your library is still available.";
        case RuntimeOutcome::HandshakeFailure:
            return L"The game process started, but the ZERO SDK connection could not be established securely.";
        case RuntimeOutcome::ReadyTimeout:
            return L"The game did not report READY within the runtime startup window. ZERO stopped the session instead of leaving it stuck.";
        case RuntimeOutcome::Hung:
            return L"ZERO lost the game heartbeat for the sustained watchdog window and ended the unresponsive session. Diagnostics were preserved.";
        case RuntimeOutcome::Crash:
            return L"ZERO contained the crash, preserved the console, and recorded available crash diagnostics and minidump information.";
        case RuntimeOutcome::UserTermination:
            return L"The game session was ended from ZERO. Your console and local platform data remain available.";
        case RuntimeOutcome::CleanExit:
            return L"The game exited normally. Session playtime and platform state have been saved.";
        default:
            return L"ZERO has returned control to the console.";
    }
}

bool IsFailure(RuntimeOutcome outcome) {
    return outcome == RuntimeOutcome::LaunchFailure ||
           outcome == RuntimeOutcome::HandshakeFailure ||
           outcome == RuntimeOutcome::ReadyTimeout ||
           outcome == RuntimeOutcome::Hung ||
           outcome == RuntimeOutcome::Crash;
}
}

void App::UpdateLaunchUx() {
    const auto outcome = runtime_.Outcome();
    if (outcome == lastRuntimeOutcome_) return;
    lastRuntimeOutcome_ = outcome;

    switch (outcome) {
        case RuntimeOutcome::Starting:
            launchUxMode_ = LaunchUxMode::Starting;
            launchError_.clear();
            break;
        case RuntimeOutcome::Running:
            launchUxMode_ = LaunchUxMode::Hidden;
            launchError_.clear();
            break;
        case RuntimeOutcome::LaunchFailure:
        case RuntimeOutcome::HandshakeFailure:
        case RuntimeOutcome::ReadyTimeout:
        case RuntimeOutcome::Hung:
        case RuntimeOutcome::Crash:
            launchUxMode_ = LaunchUxMode::Failed;
            if (overlayVisible_) SetOverlayVisible(false);
            SetWindowPos(hwnd_, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            ShowWindow(hwnd_, SW_SHOW);
            SetForegroundWindow(hwnd_);
            NotifyFocusMoved();
            break;
        case RuntimeOutcome::CleanExit:
        case RuntimeOutcome::UserTermination:
            launchUxMode_ = LaunchUxMode::Ended;
            if (overlayVisible_) SetOverlayVisible(false);
            SetWindowPos(hwnd_, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            ShowWindow(hwnd_, SW_SHOW);
            SetForegroundWindow(hwnd_);
            NotifyFocusMoved();
            break;
        default:
            break;
    }
}

void App::RetryLaunch() {
    const auto& games = registry_.Games();
    const auto it = std::find_if(games.begin(), games.end(), [this](const GameManifest& game) {
        return game.packageId == launchPackageId_;
    });
    if (it == games.end()) {
        launchError_ = L"The game is no longer present in the installed library.";
        launchUxMode_ = LaunchUxMode::Failed;
        return;
    }

    selectedGame_ = static_cast<size_t>(std::distance(games.begin(), it));
    launchUxMode_ = LaunchUxMode::Hidden;
    LaunchSelected(launchUsedResume_);
}

void App::ReturnFromLaunchUx(bool toGameDetail) {
    launchUxMode_ = LaunchUxMode::Hidden;
    launchError_.clear();
    if (toGameDetail && !launchPackageId_.empty()) {
        const auto& games = registry_.Games();
        const auto it = std::find_if(games.begin(), games.end(), [this](const GameManifest& game) {
            return game.packageId == launchPackageId_;
        });
        if (it != games.end()) {
            selectedGame_ = static_cast<size_t>(std::distance(games.begin(), it));
            navIndex_ = 1;
            NavigateTo(Page::GameDetail);
            return;
        }
    }
    navIndex_ = 0;
    NavigateTo(Page::Home);
}

void App::HandleLaunchRecoveryInput(const InputSnapshot& in) {
    if (launchUxMode_ == LaunchUxMode::Hidden) return;

    if (launchUxMode_ == LaunchUxMode::Starting) {
        if (in.back) {
            runtime_.Terminate();
            launchUxMode_ = LaunchUxMode::Ended;
            NotifyFocusMoved();
        }
        return;
    }

    if (in.select && launchUxMode_ == LaunchUxMode::Failed) {
        RetryLaunch();
        return;
    }
    if (in.action) {
        ReturnFromLaunchUx(true);
        return;
    }
    if (in.back || in.select) {
        ReturnFromLaunchUx(false);
    }
}

void App::DrawLaunchRecovery(float width, float height) {
    if (launchUxMode_ == LaunchUxMode::Hidden) return;

    ComPtr<ID2D1SolidColorBrush> veil;
    ComPtr<ID2D1SolidColorBrush> white;
    ComPtr<ID2D1SolidColorBrush> soft;
    target_->CreateSolidColorBrush(D2D1::ColorF(0x0C0C0C, 0.97f), veil.GetAddressOf());
    target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), white.GetAddressOf());
    target_->CreateSolidColorBrush(D2D1::ColorF(0x2A2A2A), soft.GetAddressOf());
    target_->FillRectangle(D2D1::RectF(0, 0, width, height), veil.Get());

    const float panelWidth = std::min(920.0f, width - 160.0f);
    const float left = (width - panelWidth) * 0.5f;
    const float top = std::max(90.0f, height * 0.5f - 260.0f);
    const auto panel = D2D1::RectF(left, top, left + panelWidth, top + 520.0f);
    DrawRoundedCard(panel, 34, soft.Get());

    DrawTextLine(L"ZERO", left + 52, top + 42, 220, 48, true, white.Get());
    DrawTextLine(launchTitle_.empty() ? L"Game session" : launchTitle_, left + 54, top + 100, panelWidth - 108, 34, false, brushMuted_.Get());

    if (launchUxMode_ == LaunchUxMode::Starting) {
        DrawTextLine(launchUsedResume_ ? L"Resuming game" : L"Starting game", left + 52, top + 170, panelWidth - 104, 58, true, white.Get());
        const auto outcome = runtime_.Outcome();
        std::wstring phase = L"Preparing secure runtime session…";
        if (runtime_.State() == RuntimeState::Launching) phase = L"Starting process and waiting for game READY…";
        DrawTextLine(phase, left + 54, top + 242, panelWidth - 108, 60, false, brushMuted_.Get());
        DrawTextLine(L"ZERO remains available if startup fails or the game does not become ready.", left + 54, top + 310, panelWidth - 108, 54, false, brushMuted_.Get());
        DrawTextLine(L"B   Cancel launch", left + 54, top + 430, 280, 34, false, white.Get());
        return;
    }

    const auto outcome = runtime_.Outcome();
    DrawTextLine(OutcomeTitle(outcome), left + 52, top + 160, panelWidth - 104, 62, true, white.Get());
    DrawTextLine(OutcomeBody(outcome), left + 54, top + 232, panelWidth - 108, 100, false, brushMuted_.Get());

    if (!launchError_.empty())
        DrawTextLine(launchError_, left + 54, top + 330, panelWidth - 108, 52, false, white.Get());

    if (launchUxMode_ == LaunchUxMode::Failed || IsFailure(outcome)) {
        const auto retry = D2D1::RectF(left + 52, top + 405, left + 250, top + 463);
        DrawRoundedCard(retry, 18, white.Get());
        DrawFocusRing(retry, 18, false);
        DrawTextLine(L"A   Try again", left + 88, top + 421, 130, 30, false, brushText_.Get());
        DrawTextLine(L"X   Game details", left + 292, top + 421, 190, 30, false, white.Get());
        DrawTextLine(L"B   Home", left + 530, top + 421, 120, 30, false, white.Get());
    } else {
        const auto home = D2D1::RectF(left + 52, top + 405, left + 250, top + 463);
        DrawRoundedCard(home, 18, white.Get());
        DrawFocusRing(home, 18, false);
        DrawTextLine(L"A   Home", left + 100, top + 421, 110, 30, false, brushText_.Get());
        DrawTextLine(L"X   Game details", left + 292, top + 421, 190, 30, false, white.Get());
    }
}

} // namespace zero
