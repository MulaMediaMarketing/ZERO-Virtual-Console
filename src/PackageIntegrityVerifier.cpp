#include "PackageIntegrityVerifier.h"
#include "PackageSecurity.h"
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>

namespace zero {
namespace {

bool isHexDigest(const std::string& value) {
    if (value.size() != 64) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isxdigit(c) != 0;
    });
}

bool utf8ToPath(const std::string& value, std::filesystem::path& path) {
    if (value.empty()) return false;
    const int chars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                          static_cast<int>(value.size()), nullptr, 0);
    if (chars <= 0) return false;
    std::wstring wide(static_cast<size_t>(chars), L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                             static_cast<int>(value.size()), wide.data(), chars)) return false;
    path = std::filesystem::path(wide).lexically_normal();
    return true;
}

bool safeManifestRelativePath(const std::filesystem::path& relative) {
    if (relative.empty() || relative.is_absolute() || relative.has_root_name() || relative.has_root_directory()) return false;
    if (relative.native().size() > PackageSecurity::kMaxRelativePathChars) return false;
    for (const auto& part : relative) {
        const auto value = part.wstring();
        if (value.empty() || value == L"." || value == L"..") return false;
    }
    return true;
}

bool parseHeaderValue(const std::string& line, const char* key, uint64_t& value) {
    const std::string token = std::string(key) + "=";
    const auto start = line.find(token);
    if (start == std::string::npos) return false;
    const auto numberStart = start + token.size();
    const auto end = line.find_first_not_of("0123456789", numberStart);
    const auto text = line.substr(numberStart, end == std::string::npos ? std::string::npos : end - numberStart);
    if (text.empty()) return false;
    try {
        size_t consumed = 0;
        const unsigned long long parsed = std::stoull(text, &consumed, 10);
        if (consumed != text.size()) return false;
        value = static_cast<uint64_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool readExpectedInventory(const std::filesystem::path& packageRoot,
                           PackageInventory& expected,
                           std::wstring& error) {
    expected = {};
    const auto integrityPath = packageRoot / L"zero.integrity.sha256";
    std::error_code ec;
    if (!std::filesystem::exists(integrityPath, ec) || !std::filesystem::is_regular_file(integrityPath, ec)) {
        error = L"ZERO blocked launch because the package integrity manifest is missing.";
        return false;
    }
    if (std::filesystem::file_size(integrityPath, ec) > PackageSecurity::kMaxManifestBytes || ec) {
        error = L"ZERO blocked launch because the package integrity manifest is invalid or too large.";
        return false;
    }

    std::ifstream file(integrityPath, std::ios::binary);
    if (!file) {
        error = L"ZERO could not open the package integrity manifest.";
        return false;
    }

    std::string versionLine;
    std::string summaryLine;
    if (!std::getline(file, versionLine) || versionLine != "# ZERO package integrity v1" ||
        !std::getline(file, summaryLine) || summaryLine.rfind("# files=", 0) != 0) {
        error = L"ZERO blocked launch because the package integrity manifest format is invalid.";
        return false;
    }

    uint64_t declaredFiles = 0;
    uint64_t declaredBytes = 0;
    if (!parseHeaderValue(summaryLine, "files", declaredFiles) ||
        !parseHeaderValue(summaryLine, "bytes", declaredBytes) ||
        declaredFiles > PackageSecurity::kMaxFiles || declaredBytes > PackageSecurity::kMaxPackageBytes) {
        error = L"ZERO blocked launch because the package integrity manifest summary is invalid.";
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        const auto firstSep = line.find("  ");
        const auto secondSep = firstSep == std::string::npos ? std::string::npos : line.find("  ", firstSep + 2);
        if (firstSep == std::string::npos || secondSep == std::string::npos) {
            error = L"ZERO blocked launch because a package integrity record is malformed.";
            return false;
        }

        const std::string digest = line.substr(0, firstSep);
        const std::string sizeText = line.substr(firstSep + 2, secondSep - (firstSep + 2));
        const std::string pathText = line.substr(secondSep + 2);
        if (!isHexDigest(digest) || sizeText.empty() || pathText.empty()) {
            error = L"ZERO blocked launch because a package integrity record is invalid.";
            return false;
        }

        uint64_t size = 0;
        try {
            size_t consumed = 0;
            const unsigned long long parsed = std::stoull(sizeText, &consumed, 10);
            if (consumed != sizeText.size() || parsed > PackageSecurity::kMaxSingleFileBytes) throw std::out_of_range("size");
            size = static_cast<uint64_t>(parsed);
        } catch (...) {
            error = L"ZERO blocked launch because a package integrity file size is invalid.";
            return false;
        }

        std::filesystem::path relative;
        if (!utf8ToPath(pathText, relative) || !safeManifestRelativePath(relative)) {
            error = L"ZERO blocked launch because the integrity manifest contains an unsafe package path.";
            return false;
        }

        expected.files.push_back({relative, size, digest});
        if (expected.files.size() > PackageSecurity::kMaxFiles) {
            error = L"ZERO blocked launch because the integrity manifest exceeds the supported file count.";
            return false;
        }
        if (expected.totalBytes > PackageSecurity::kMaxPackageBytes - size) {
            error = L"ZERO blocked launch because the integrity manifest exceeds the supported package size.";
            return false;
        }
        expected.totalBytes += size;
    }

    expected.fileCount = static_cast<uint32_t>(expected.files.size());
    if (expected.fileCount != declaredFiles || expected.totalBytes != declaredBytes) {
        error = L"ZERO blocked launch because the integrity manifest summary does not match its records.";
        return false;
    }

    std::sort(expected.files.begin(), expected.files.end(), [](const auto& a, const auto& b) {
        return _wcsicmp(a.relativePath.generic_wstring().c_str(), b.relativePath.generic_wstring().c_str()) < 0;
    });
    for (size_t i = 1; i < expected.files.size(); ++i) {
        if (_wcsicmp(expected.files[i - 1].relativePath.generic_wstring().c_str(),
                     expected.files[i].relativePath.generic_wstring().c_str()) == 0) {
            error = L"ZERO blocked launch because the integrity manifest contains duplicate package paths.";
            return false;
        }
    }
    return true;
}

} // namespace

bool PackageIntegrityVerifier::Verify(const std::filesystem::path& packageRoot, std::wstring& error) {
    PackageInventory expected;
    if (!readExpectedInventory(packageRoot, expected, error)) return false;

    PackageInventory actual;
    if (!PackageSecurity::BuildInventory(packageRoot, actual, error)) {
        if (error.empty()) error = L"ZERO blocked launch because the installed package could not be validated.";
        return false;
    }

    std::wstring equivalentError;
    if (!PackageSecurity::Equivalent(expected, actual, equivalentError)) {
        error = L"ZERO blocked launch because installed game files changed after import. Re-import the game to restore package integrity.";
        return false;
    }
    return true;
}

} // namespace zero
