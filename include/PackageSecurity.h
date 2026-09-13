#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace zero {

struct PackageFileRecord {
    std::filesystem::path relativePath;
    uint64_t sizeBytes{0};
    std::string sha256;
};

struct PackageInventory {
    uint64_t totalBytes{0};
    uint32_t fileCount{0};
    std::vector<PackageFileRecord> files;
};

class PackageSecurity {
public:
    static constexpr uint32_t kMaxFiles = 20000;
    static constexpr uint64_t kMaxPackageBytes = 50ull * 1024ull * 1024ull * 1024ull;
    static constexpr uint64_t kMaxSingleFileBytes = 4ull * 1024ull * 1024ull * 1024ull;
    static constexpr uint64_t kMaxManifestBytes = 1024ull * 1024ull;
    static constexpr size_t kMaxRelativePathChars = 512;

    static bool BuildInventory(const std::filesystem::path& root,
                               PackageInventory& inventory,
                               std::wstring& error);
    static bool Equivalent(const PackageInventory& expected,
                           const PackageInventory& staged,
                           std::wstring& error);
    static bool WriteIntegrityManifest(const std::filesystem::path& packageRoot,
                                       const PackageInventory& inventory,
                                       std::wstring& error);
};

} // namespace zero
