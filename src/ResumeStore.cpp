#include "ResumeStore.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <shlobj.h>
#include <sstream>

namespace zero {
namespace {

std::filesystem::path root() {
    PWSTR p = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &p))) {
        std::filesystem::path out = std::filesystem::path(p) / "ZERO" / "Resume";
        CoTaskMemFree(p);
        return out;
    }
    return std::filesystem::temp_directory_path() / "ZERO" / "Resume";
}

std::string escapeJson(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (unsigned char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += static_cast<char>(c); break;
        }
    }
    return out;
}

std::string utcNow() {
    const auto now = std::chrono::system_clock::now();
    const auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
    gmtime_s(&utc, &tt);
    std::ostringstream os;
    os << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return os.str();
}

std::string readField(const std::string& text, const std::string& key) {
    const std::string token = "\"" + key + "\"";
    auto p = text.find(token);
    if (p == std::string::npos) return {};
    p = text.find(':', p + token.size());
    if (p == std::string::npos) return {};
    p = text.find('"', p + 1);
    if (p == std::string::npos) return {};
    ++p;
    std::string out;
    bool esc = false;
    for (; p < text.size(); ++p) {
        char c = text[p];
        if (esc) {
            if (c == 'n') out += '\n';
            else if (c == 'r') out += '\r';
            else if (c == 't') out += '\t';
            else out += c;
            esc = false;
        } else if (c == '\\') {
            esc = true;
        } else if (c == '"') {
            break;
        } else {
            out += c;
        }
    }
    return out;
}

bool safeId(const std::string& id) {
    if (id.empty() || id.size() > 160) return false;
    for (unsigned char c : id) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_')) return false;
    }
    return id.find("..") == std::string::npos;
}

} // namespace

std::filesystem::path ResumeStore::PathFor(const std::string& packageId) const {
    return root() / std::filesystem::path(std::wstring(packageId.begin(), packageId.end()) + L".json");
}

bool ResumeStore::Save(const ResumeMetadata& metadata, std::wstring& error) const {
    if (!safeId(metadata.packageId)) {
        error = L"Invalid package ID for Resume metadata.";
        return false;
    }
    std::error_code ec;
    std::filesystem::create_directories(root(), ec);
    if (ec) {
        error = L"ZERO could not create the Resume metadata directory.";
        return false;
    }

    const auto path = PathFor(metadata.packageId);
    const auto tmp = path.wstring() + L".tmp";
    std::ofstream f(std::filesystem::path(tmp), std::ios::binary | std::ios::trunc);
    if (!f) {
        error = L"ZERO could not write Resume metadata.";
        return false;
    }

    f << "{\n"
      << "  \"schema\": 1,\n"
      << "  \"package_id\": \"" << escapeJson(metadata.packageId) << "\",\n"
      << "  \"activity_id\": \"" << escapeJson(metadata.activityId) << "\",\n"
      << "  \"display_label\": \"" << escapeJson(metadata.displayLabel) << "\",\n"
      << "  \"payload\": \"" << escapeJson(metadata.payload) << "\",\n"
      << "  \"updated_at\": \"" << escapeJson(metadata.updatedAtUtc.empty() ? utcNow() : metadata.updatedAtUtc) << "\"\n"
      << "}\n";
    f.close();

    std::filesystem::rename(std::filesystem::path(tmp), path, ec);
    if (ec) {
        std::filesystem::remove(path, ec);
        ec.clear();
        std::filesystem::rename(std::filesystem::path(tmp), path, ec);
    }
    if (ec) {
        error = L"ZERO could not commit Resume metadata.";
        return false;
    }
    return true;
}

std::optional<ResumeMetadata> ResumeStore::Load(const std::string& packageId) const {
    if (!safeId(packageId)) return std::nullopt;
    std::ifstream f(PathFor(packageId), std::ios::binary);
    if (!f) return std::nullopt;
    std::ostringstream ss;
    ss << f.rdbuf();
    const auto text = ss.str();
    ResumeMetadata m;
    m.packageId = readField(text, "package_id");
    m.activityId = readField(text, "activity_id");
    m.displayLabel = readField(text, "display_label");
    m.payload = readField(text, "payload");
    m.updatedAtUtc = readField(text, "updated_at");
    if (m.packageId != packageId) return std::nullopt;
    return m;
}

bool ResumeStore::Clear(const std::string& packageId, std::wstring& error) const {
    if (!safeId(packageId)) {
        error = L"Invalid package ID for Resume metadata.";
        return false;
    }
    std::error_code ec;
    std::filesystem::remove(PathFor(packageId), ec);
    if (ec) {
        error = L"ZERO could not clear Resume metadata.";
        return false;
    }
    return true;
}

} // namespace zero
