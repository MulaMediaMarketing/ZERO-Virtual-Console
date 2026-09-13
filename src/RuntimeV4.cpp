#include "RuntimeV4.h"

namespace zero {

bool RuntimeV4::Launch(const GameManifest& game, std::wstring& error) {
    v3_.SetAchievementCallback([this](const std::string& id, const std::string& title) {
        std::wstring ignored;
        UnlockAchievement(id, title, ignored);
    });
    if (!v3_.Launch(game, error)) return false;
    activePackageId_ = game.packageId;
    activeSessionId_ = v3_.Info().sessionId;
    sessionRecorded_ = false;
    crashReported_ = false;
    return true;
}

void RuntimeV4::Poll() {
    v3_.Poll();
    const auto state = v3_.State();

    if (!crashReported_ && state == RuntimeState::Crashed) {
        const auto& info = v3_.Info();
        CrashReport report;
        report.sessionId = info.sessionId;
        report.packageId = info.packageId;
        report.title = info.title;
        report.version = info.version;
        report.executable = info.executable.string();
        report.processId = info.processId;
        report.exitCode = v3_.ExitCode();
        report.playtimeSeconds = v3_.PlaytimeSeconds();
        report.forcedTermination = info.forcedTermination;
        std::wstring ignored;
        crashReports_.Save(report, ignored);
        crashReported_ = true;
    }

    if (!sessionRecorded_ && (state == RuntimeState::Exited || state == RuntimeState::Crashed || state == RuntimeState::Failed)) {
        std::wstring ignored;
        stateStore_.RecordSession(activePackageId_, activeSessionId_, v3_.PlaytimeSeconds(), v3_.ExitCode(), state == RuntimeState::Crashed, ignored);
        sessionRecorded_ = true;
    }
}

void RuntimeV4::Terminate() {
    if (!v3_.IsActive()) return;
    v3_.Terminate();
    Poll();
}

bool RuntimeV4::UnlockAchievement(const std::string& achievementId,
                                  const std::string& title,
                                  std::wstring& error) {
    if (activePackageId_.empty()) {
        error = L"ZERO cannot unlock an achievement without an active game package.";
        return false;
    }
    if (achievementId.empty() || achievementId.size() > 160 || title.empty() || title.size() > 256) {
        error = L"The achievement identity is invalid.";
        return false;
    }
    return achievements_.Unlock(activePackageId_, achievementId, title, error);
}

} // namespace zero
