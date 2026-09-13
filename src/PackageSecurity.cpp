#include "PackageSecurity.h"
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <cwctype>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <vector>
#pragma comment(lib, "bcrypt.lib")

namespace zero {
namespace {

bool reservedComponent(const std::wstring& component) {
    if (component.empty() || component == L"." || component == L"..") return true;
    std::wstring base = component;
    const auto dot = base.find(L'.');
    if (dot != std::wstring::npos) base.resize(dot);
    std::transform(base.begin(), base.end(), base.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towupper(c)); });
    static const std::set<std::wstring> names = {
        L"CON", L"PRN", L"AUX", L"NUL",
        L"COM1", L"COM2", L"COM3", L"COM4", L"COM5", L"COM6", L"COM7", L"COM8", L"COM9",
        L"LPT1", L"LPT2", L"LPT3", L"LPT4", L"LPT5", L"LPT6", L"LPT7", L"LPT8", L"LPT9"
    };
    if (names.contains(base)) return true;
    const wchar_t last = component.back();
    return last == L' ' || last == L'.';
}

bool safeRelativePath(const std::filesystem::path& relative) {
    if (relative.empty() || relative.is_absolute() || relative.has_root_name() || relative.has_root_directory()) return false;
    if (relative.native().size() > PackageSecurity::kMaxRelativePathChars) return false;
    for (const auto& part : relative) {
        const auto value = part.wstring();
        if (value == L".." || value == L"." || reservedComponent(value)) return false;
    }
    return true;
}

bool isGeneratedIntegrityManifest(const std::filesystem::path& relative) {
    return _wcsicmp(relative.generic_wstring().c_str(), L"zero.integrity.sha256") == 0;
}

bool isReparsePoint(const std::filesystem::path& path) {
    const DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

bool hasMultipleHardLinks(const std::filesystem::path& path, std::wstring& error) {
    HANDLE file = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        error = L"ZERO could not inspect a package file for hard-link safety.";
        return true;
    }
    BY_HANDLE_FILE_INFORMATION info{};
    const BOOL ok = GetFileInformationByHandle(file, &info);
    CloseHandle(file);
    if (!ok) {
        error = L"ZERO could not inspect package file metadata.";
        return true;
    }
    return info.nNumberOfLinks > 1;
}

bool sha256File(const std::filesystem::path& path, std::string& digest, std::wstring& error) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD objectBytes = 0, hashBytes = 0, cb = 0;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) {
        error = L"ZERO could not initialize SHA-256 validation.";
        return false;
    }
    auto closeAlg = [&] { if (alg) BCryptCloseAlgorithmProvider(alg, 0); };
    if (BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectBytes), sizeof(objectBytes), &cb, 0) != 0 ||
        BCryptGetProperty(alg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashBytes), sizeof(hashBytes), &cb, 0) != 0) {
        closeAlg(); error = L"ZERO could not query SHA-256 provider properties."; return false;
    }
    std::vector<UCHAR> object(objectBytes);
    std::vector<UCHAR> output(hashBytes);
    if (BCryptCreateHash(alg, &hash, object.data(), objectBytes, nullptr, 0, 0) != 0) {
        closeAlg(); error = L"ZERO could not create SHA-256 state."; return false;
    }
    auto cleanup = [&] { if (hash) BCryptDestroyHash(hash); closeAlg(); };

    std::ifstream f(path, std::ios::binary);
    if (!f) { cleanup(); error = L"ZERO could not open a package file for integrity validation."; return false; }
    std::vector<char> buffer(1024 * 1024);
    while (f) {
        f.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto count = f.gcount();
        if (count > 0 && BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer.data()), static_cast<ULONG>(count), 0) != 0) {
            cleanup(); error = L"ZERO could not hash a package file."; return false;
        }
    }
    if (!f.eof()) { cleanup(); error = L"ZERO encountered an I/O failure while hashing a package file."; return false; }
    if (BCryptFinishHash(hash, output.data(), hashBytes, 0) != 0) {
        cleanup(); error = L"ZERO could not finalize package SHA-256 validation."; return false;
    }
    cleanup();

    std::ostringstream os;
    os << std::hex << std::setfill('0');
    for (auto byte : output) os << std::setw(2) << static_cast<unsigned>(byte);
    digest = os.str();
    return true;
}

std::string utf8Path(const std::filesystem::path& path) {
    const auto wide = path.generic_wstring();
    if (wide.empty()) return {};
    const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return {};
    std::string out(static_cast<size_t>(bytes), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), static_cast<int>(wide.size()), out.data(), bytes, nullptr, nullptr);
    return out;
}

} // namespace

bool PackageSecurity::BuildInventory(const std::filesystem::path& root,
                                     PackageInventory& inventory,
                                     std::wstring& error) {
    inventory = {};
    std::error_code ec;
    if (!std::filesystem::exists(root, ec) || !std::filesystem::is_directory(root, ec)) {
        error = L"The package root does not exist or is not a directory.";
        return false;
    }
    if (isReparsePoint(root)) {
        error = L"ZERO packages cannot use a reparse-point package root.";
        return false;
    }

    for (std::filesystem::recursive_directory_iterator it(root, std::filesystem::directory_options::none, ec), end;
         !ec && it != end; it.increment(ec)) {
        const auto path = it->path();
        const auto relative = std::filesystem::relative(path, root, ec);
        if (ec || !safeRelativePath(relative)) {
            error = L"ZERO rejected an unsafe package path.";
            return false;
        }
        if (isReparsePoint(path)) {
            error = L"ZERO packages cannot contain symbolic links, junctions, or reparse points.";
            return false;
        }
        if (it->is_directory(ec)) continue;
        if (!it->is_regular_file(ec)) {
            error = L"ZERO packages may contain regular files and directories only.";
            return false;
        }
        if (isGeneratedIntegrityManifest(relative)) continue;
        if (++inventory.fileCount > kMaxFiles) {
            error = L"ZERO package exceeds the maximum supported file count.";
            return false;
        }
        const uint64_t size = it->file_size(ec);
        if (ec) { error = L"ZERO could not read package file size metadata."; return false; }
        if (size > kMaxSingleFileBytes) {
            error = L"ZERO package contains a file larger than the per-file import limit.";
            return false;
        }
        if (inventory.totalBytes > kMaxPackageBytes - size) {
            error = L"ZERO package exceeds the maximum supported package size.";
            return false;
        }
        inventory.totalBytes += size;

        std::wstring hardLinkError;
        if (hasMultipleHardLinks(path, hardLinkError)) {
            error = hardLinkError.empty() ? L"ZERO packages cannot contain multiply-linked files." : hardLinkError;
            return false;
        }

        std::string digest;
        if (!sha256File(path, digest, error)) return false;
        inventory.files.push_back({relative.lexically_normal(), size, std::move(digest)});
    }
    if (ec) { error = L"ZERO could not safely enumerate the entire package."; return false; }

    std::sort(inventory.files.begin(), inventory.files.end(), [](const auto& a, const auto& b) {
        return _wcsicmp(a.relativePath.generic_wstring().c_str(), b.relativePath.generic_wstring().c_str()) < 0;
    });

    const auto manifest = root / L"zero.manifest.json";
    if (!std::filesystem::exists(manifest, ec) || std::filesystem::file_size(manifest, ec) > kMaxManifestBytes) {
        error = L"ZERO manifest is missing or exceeds the maximum supported size.";
        return false;
    }
    return true;
}

bool PackageSecurity::Equivalent(const PackageInventory& expected,
                                 const PackageInventory& staged,
                                 std::wstring& error) {
    if (expected.fileCount != staged.fileCount || expected.totalBytes != staged.totalBytes ||
        expected.files.size() != staged.files.size()) {
        error = L"The staged package differs from the validated source package.";
        return false;
    }
    for (size_t i = 0; i < expected.files.size(); ++i) {
        const auto& a = expected.files[i];
        const auto& b = staged.files[i];
        if (_wcsicmp(a.relativePath.generic_wstring().c_str(), b.relativePath.generic_wstring().c_str()) != 0 ||
            a.sizeBytes != b.sizeBytes || a.sha256 != b.sha256) {
            error = L"Package integrity verification failed after staging.";
            return false;
        }
    }
    return true;
}

bool PackageSecurity::WriteIntegrityManifest(const std::filesystem::path& packageRoot,
                                             const PackageInventory& inventory,
                                             std::wstring& error) {
    const auto path = packageRoot / L"zero.integrity.sha256";
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) { error = L"ZERO could not create the package integrity manifest."; return false; }
    f << "# ZERO package integrity v1\n";
    f << "# files=" << inventory.fileCount << " bytes=" << inventory.totalBytes << "\n";
    for (const auto& record : inventory.files) {
        const auto relative = utf8Path(record.relativePath);
        if (relative.empty()) { error = L"ZERO could not encode a package path for the integrity manifest."; return false; }
        f << record.sha256 << "  " << record.sizeBytes << "  " << relative << "\n";
    }
    f.flush();
    if (!f.good()) { error = L"ZERO could not finalize the package integrity manifest."; return false; }
    return true;
}

} // namespace zero
