#pragma once
#include <windows.h>
#include <filesystem>
#include <string>

namespace zero {

struct MiniDumpResult {
    bool written{false};
    std::filesystem::path path;
    DWORD windowsError{0};
    std::string note;
};

class MiniDumpWriter {
public:
    MiniDumpResult Capture(HANDLE process,
                           DWORD processId,
                           const std::string& packageId,
                           const std::string& sessionId) const;
};

} // namespace zero
