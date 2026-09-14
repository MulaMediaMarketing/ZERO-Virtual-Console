#pragma once

#include "ZeroTypes.h"
#include <filesystem>
#include <string>

namespace zero {

struct PackageManifestParseResult {
    GameManifest manifest;
    std::wstring error;
    bool valid{false};
};

class PackageManifestParser {
public:
    static PackageManifestParseResult ParseFile(const std::filesystem::path& manifestPath,
                                                const std::filesystem::path& packageRoot);
};

} // namespace zero
