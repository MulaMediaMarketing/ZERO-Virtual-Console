#include "RuntimeV4.h"

namespace zero {

bool RuntimeV4::Launch(const GameManifest& game, std::wstring& error) {
    if (!v3_.Launch(game, error)) return false;
    activePackageId_ = game.packageId;
    activeSessionId_ = v3_.Info().sessionId;
    sessionRecorded_ = false;
    return true;
}

void RuntimeV4::Poll() {
    v3_.Poll();
    const auto state = v3_.State();
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

} // namespace zero
