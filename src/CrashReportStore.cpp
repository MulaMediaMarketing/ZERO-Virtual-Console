#include "CrashReportStore.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <shlobj.h>
#include <sstream>

namespace zero {
namespace {
std::string esc(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}
std::string utcNow() {
    const auto now = std::chrono::system_clock::now();
    const auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    gmtime_s(&tm, &tt);
    std::ostringstream os;
    os << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return os.str();
}
}

std::filesystem::path CrashReportStore::Root() const {
    PWSTR p = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &p))) {
        std::filesystem::path root = std::filesystem::path(p) / "ZERO" / "CrashReports";
        CoTaskMemFree(p);
        return root;
    }
    return std::filesystem::temp_directory_path() / "ZERO" / "CrashReports";
}

bool CrashReportStore::Save(const CrashReport& report, std::wstring& error) const {
    std::error_code ec;
    const auto dir = Root() / std::filesystem::path(report.packageId.begin(), report.packageId.end());
    std::filesystem::create_directories(dir, ec);
    if (ec) { error = L"ZERO could not create the crash-report directory."; return false; }

    const auto file = dir / (std::filesystem::path(report.sessionId.begin(), report.sessionId.end()).wstring() + L".json");
    const auto tmp = file.wstring() + L".tmp";
    std::ofstream f(std::filesystem::path(tmp), std::ios::binary | std::ios::trunc);
    if (!f) { error = L"ZERO could not create the crash report."; return false; }

    const std::string stamp = report.timestampUtc.empty() ? utcNow() : report.timestampUtc;
    f << "{\n"
      << "  \"schema\": 2,\n"
      << "  \"session_id\": \"" << esc(report.sessionId) << "\",\n"
      << "  \"package_id\": \"" << esc(report.packageId) << "\",\n"
      << "  \"title\": \"" << esc(report.title) << "\",\n"
      << "  \"version\": \"" << esc(report.version) << "\",\n"
      << "  \"executable\": \"" << esc(report.executable) << "\",\n"
      << "  \"timestamp_utc\": \"" << esc(stamp) << "\",\n"
      << "  \"outcome\": \"" << esc(report.outcome) << "\",\n"
      << "  \"process_id\": " << report.processId << ",\n"
      << "  \"exit_code\": " << report.exitCode << ",\n"
      << "  \"playtime_seconds\": " << report.playtimeSeconds << ",\n"
      << "  \"forced_termination\": " << (report.forcedTermination ? "true" : "false") << ",\n"
      << "  \"minidump_written\": " << (report.miniDumpWritten ? "true" : "false") << ",\n"
      << "  \"minidump_path\": \"" << esc(report.miniDumpPath) << "\",\n"
      << "  \"minidump_error\": " << report.miniDumpError << ",\n"
      << "  \"minidump_note\": \"" << esc(report.miniDumpNote) << "\"\n"
      << "}\n";
    f.close();

    std::filesystem::rename(std::filesystem::path(tmp), file, ec);
    if (ec) {
        std::filesystem::remove(file, ec);
        ec.clear();
        std::filesystem::rename(std::filesystem::path(tmp), file, ec);
    }
    if (ec) { error = L"ZERO could not finalize the crash report."; return false; }
    return true;
}

} // namespace zero
