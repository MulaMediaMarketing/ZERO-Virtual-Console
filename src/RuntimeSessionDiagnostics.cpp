#include "RuntimeSession.h"

namespace zero {

bool RuntimeSession::HasAbnormalExit() const noexcept {
    if (!process_.hProcess || !IsActive()) return false;
    if (WaitForSingleObject(process_.hProcess, 0) != WAIT_OBJECT_0) return false;
    DWORD code = 0;
    if (!GetExitCodeProcess(process_.hProcess, &code)) return true;
    return code != 0;
}

bool RuntimeSession::CaptureDiagnosticDump() {
    if (info_.miniDumpWritten) return true;
    if (!process_.hProcess || process_.dwProcessId == 0) {
        info_.miniDumpNote = "Process was unavailable for diagnostic dump capture.";
        return false;
    }

    const auto result = miniDumpWriter_.Capture(
        process_.hProcess, process_.dwProcessId, info_.packageId, info_.sessionId);
    info_.miniDumpWritten = result.written;
    info_.miniDumpPath = result.path;
    info_.miniDumpError = result.windowsError;
    info_.miniDumpNote = result.note;
    return result.written;
}

} // namespace zero
