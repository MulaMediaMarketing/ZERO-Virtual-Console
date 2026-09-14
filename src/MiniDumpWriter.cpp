#include "MiniDumpWriter.h"
#include "PlatformPaths.h"
#include <dbghelp.h>

namespace zero {
namespace {

bool safeId(const std::string& id) {
    if (id.empty() || id.size() > 160 || id.find("..") != std::string::npos) return false;
    for (unsigned char c : id) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_';
        if (!ok) return false;
    }
    return true;
}

} // namespace

MiniDumpResult MiniDumpWriter::Capture(HANDLE process,
                                       DWORD processId,
                                       const std::string& packageId,
                                       const std::string& sessionId) const {
    MiniDumpResult result;
    if (!process || process == INVALID_HANDLE_VALUE || processId == 0) {
        result.note = "Process handle was unavailable for dump capture.";
        return result;
    }
    if (!safeId(packageId) || !safeId(sessionId)) {
        result.note = "Package or session identity was unsafe for dump storage.";
        return result;
    }

    std::error_code ec;
    const auto dir = PlatformPaths::CrashReportsRoot() / std::filesystem::path(packageId.begin(), packageId.end());
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        result.note = "Crash-report directory creation failed.";
        return result;
    }

    result.path = dir / (std::filesystem::path(sessionId.begin(), sessionId.end()).wstring() + L".dmp");
    HANDLE file = CreateFileW(result.path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        result.windowsError = GetLastError();
        result.note = "Minidump file creation failed.";
        return result;
    }

    const MINIDUMP_TYPE type = static_cast<MINIDUMP_TYPE>(
        MiniDumpWithThreadInfo |
        MiniDumpWithUnloadedModules |
        MiniDumpWithIndirectlyReferencedMemory |
        MiniDumpWithProcessThreadData);

    const BOOL ok = MiniDumpWriteDump(process, processId, file, type, nullptr, nullptr, nullptr);
    if (!ok) {
        result.windowsError = GetLastError();
        result.note = "MiniDumpWriteDump failed.";
    } else {
        result.written = true;
        result.note = "Minidump captured.";
    }
    CloseHandle(file);

    if (!result.written) {
        std::filesystem::remove(result.path, ec);
    }
    return result;
}

} // namespace zero
