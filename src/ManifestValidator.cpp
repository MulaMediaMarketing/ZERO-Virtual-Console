#include "ManifestValidator.h"
#include <windows.h>
#include <cwchar>

namespace zero {
namespace {

bool isReparsePoint(const std::filesystem::path& path) noexcept {
    const DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

bool safeOptionalAsset(const std::filesystem::path& asset,
                       const std::filesystem::path& root) noexcept {
    if (asset.empty()) return true;
    std::error_code ec;
    if (!std::filesystem::exists(asset, ec) || !std::filesystem::is_regular_file(asset, ec)) return false;
    if (isReparsePoint(asset)) return false;
    return ManifestValidator::IsPathInside(asset, root);
}

} // namespace

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
    if (m.minimumRuntimeMajor < 1 || m.minimumRuntimeMajor > 4) {
        result.error = m.minimumRuntimeMajor > 4
            ? L"This game requires a newer ZERO Runtime."
            : L"Manifest minimum_runtime_major is invalid.";
        return result;
    }
    if (m.root.empty() || m.executable.empty()) { result.error = L"Game root or executable is missing."; return result; }
    if (isReparsePoint(m.root) || isReparsePoint(m.executable)) {
        result.error = L"ZERO package roots and executables cannot be reparse points.";
        return result;
    }
    if (!std::filesystem::exists(m.executable) || !std::filesystem::is_regular_file(m.executable)) {
        result.error = L"Game executable does not exist."; return result;
    }
    if (_wcsicmp(m.executable.extension().c_str(), L".exe") != 0) {
        result.error = L"ZERO currently supports native Windows .exe payloads only."; return result;
    }
    if (!IsPathInside(m.executable, m.root)) {
        result.error = L"Game executable must remain inside the registered package directory."; return result;
    }
    if (!safeOptionalAsset(m.heroImage, m.root)) {
        result.error = L"Hero artwork is missing, invalid, or outside the package directory."; return result;
    }
    if (!safeOptionalAsset(m.iconImage, m.root)) {
        result.error = L"Icon artwork is missing, invalid, or outside the package directory."; return result;
    }
    if (!safeOptionalAsset(m.logoImage, m.root)) {
        result.error = L"Logo artwork is missing, invalid, or outside the package directory."; return result;
    }
    result.valid = true;
    return result;
}

} // namespace zero
