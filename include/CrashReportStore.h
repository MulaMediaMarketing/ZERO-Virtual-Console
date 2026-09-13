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
    uint32_t processId{0};
    uint32_t exitCode{0};
    uint64_t playtimeSeconds{0};
    bool forcedTermination{false};
};

class CrashReportStore {
public:
    bool Save(const CrashReport& report, std::wstring& error) const;
private:
    std::filesystem::path Root() const;
};

} // namespace zero
