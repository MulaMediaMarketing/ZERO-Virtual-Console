#include "ZeroSdk.h"
#include <windows.h>
#include <dbghelp.h>
#include <atomic>
#include <iterator>

namespace zero::sdk::detail {
namespace {

wchar_t gDumpPath[32768]{};
std::atomic<bool> gInstalled{false};
LPTOP_LEVEL_EXCEPTION_FILTER gPreviousFilter = nullptr;

LONG WINAPI ZeroUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionPointers) {
    if (gDumpPath[0] != L'\0') {
        HANDLE file = CreateFileW(gDumpPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION exceptionInfo{};
            exceptionInfo.ThreadId = GetCurrentThreadId();
            exceptionInfo.ExceptionPointers = exceptionPointers;
            exceptionInfo.ClientPointers = FALSE;

            const MINIDUMP_TYPE type = static_cast<MINIDUMP_TYPE>(
                MiniDumpWithThreadInfo |
                MiniDumpWithUnloadedModules |
                MiniDumpWithIndirectlyReferencedMemory |
                MiniDumpWithProcessThreadData);

            MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, type,
                              &exceptionInfo, nullptr, nullptr);
            CloseHandle(file);
        }
    }

    if (gPreviousFilter && gPreviousFilter != ZeroUnhandledExceptionFilter) {
        return gPreviousFilter(exceptionPointers);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

} // namespace

bool InstallCrashHandler() noexcept {
    bool expected = false;
    if (!gInstalled.compare_exchange_strong(expected, true)) return true;

    const DWORD n = GetEnvironmentVariableW(L"ZERO_CRASH_DUMP_PATH", gDumpPath,
                                             static_cast<DWORD>(std::size(gDumpPath)));
    if (!n || n >= std::size(gDumpPath)) {
        gDumpPath[0] = L'\0';
        gInstalled.store(false);
        return false;
    }

    gPreviousFilter = SetUnhandledExceptionFilter(ZeroUnhandledExceptionFilter);
    return true;
}

} // namespace zero::sdk::detail
