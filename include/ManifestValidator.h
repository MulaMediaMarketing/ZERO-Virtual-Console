#pragma once
#include "ZeroTypes.h"
#include <filesystem>
#include <string>

namespace zero {

struct ManifestValidationResult {
    bool valid{false};
    std::wstring error;
};

class ManifestValidator {
public:
    static ManifestValidationResult Validate(const GameManifest& manifest);
    static bool IsSafePackageId(const std::string& packageId) noexcept;
    static bool IsPathInside(const std::filesystem::path& child,
                             const std::filesystem::path& parent) noexcept;
};

} // namespace zero
