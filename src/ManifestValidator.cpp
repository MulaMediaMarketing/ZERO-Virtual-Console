#include "ManifestValidator.h"
#include <cwchar>

namespace zero {

bool ManifestValidator::IsSafePackageId(const std::string& id) noexcept {
    if (id.empty() || id.size() > 160 || id.find("..") != std::string::npos) return false;
    for (unsigned char c : id) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_';
        if (!ok) return false;
    }
    return true;
}

bool ManifestValidator::IsPathInside(const std::filesystem::path& child,
                                     const std::filesystem::path& parent) noexcept {
    std::error_code ec;
    const auto c = std::filesystem::weakly_canonical(child, ec);
    if (ec) return false;
    const auto p = std::filesystem::weakly_canonical(parent, ec);
    if (ec) return false;
    auto ci = c.begin();
    for (auto pi = p.begin(); pi != p.end(); ++pi, ++ci) {
        if (ci == c.end() || _wcsicmp(ci->c_str(), pi->c_str()) != 0) return false;
    }
    return true;
}

ManifestValidationResult ManifestValidator::Validate(const GameManifest& m) {
    ManifestValidationResult result;
    if (m.schemaVersion != 1) { result.error = L"Unsupported ZERO manifest schema."; return result; }
    if (!IsSafePackageId(m.packageId)) { result.error = L"Invalid package_id."; return result; }
    if (m.title.empty()) { result.error = L"Game title is required."; return result; }
    if (m.version.empty()) { result.error = L"Game version is required."; return result; }
    if (m.minimumRuntimeMajor > 4) { result.error = L"This game requires a newer ZERO Runtime."; return result; }
    if (m.root.empty() || m.executable.empty()) { result.error = L"Game root or executable is missing."; return result; }
    if (!std::filesystem::exists(m.executable) || !std::filesystem::is_regular_file(m.executable)) {
        result.error = L"Game executable does not exist."; return result;
    }
    if (_wcsicmp(m.executable.extension().c_str(), L".exe") != 0) {
        result.error = L"ZERO currently supports native Windows .exe payloads only."; return result;
    }
    if (!IsPathInside(m.executable, m.root)) {
        result.error = L"Game executable must remain inside the registered package directory."; return result;
    }
    if (!m.heroImage.empty() && !IsPathInside(m.heroImage, m.root)) {
        result.error = L"Hero artwork must remain inside the package directory."; return result;
    }
    if (!m.iconImage.empty() && !IsPathInside(m.iconImage, m.root)) {
        result.error = L"Icon artwork must remain inside the package directory."; return result;
    }
    result.valid = true;
    return result;
}

} // namespace zero
