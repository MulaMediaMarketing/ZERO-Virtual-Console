#pragma once
#include <cstdint>
#include <filesystem>
#include <string>

namespace zero {

struct CrashReport {
    std::string sessionId;
    std::string packageId;
    std::string title;
    std::string version;
    std::string executable;
    std::string timestampUtc;
    std::string outcome;
    std::string miniDumpPath;
    std::string miniDumpNote;
    uint32_t processId{0};
    uint32_t exitCode{0};
    uint32_t miniDumpError{0};
    uint64_t playtimeSeconds{0};
    bool forcedTermination{false};
    bool miniDumpWritten{false};
};

class CrashReportStore {
public:
    bool Save(const CrashReport& report, std::wstring& error) const;
private:
    std::filesystem::path Root() const;
};

} // namespace zero
