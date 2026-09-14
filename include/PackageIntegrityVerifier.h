#pragma once
#include <filesystem>
#include <string>

namespace zero {

class PackageIntegrityVerifier {
public:
    static bool Verify(const std::filesystem::path& packageRoot, std::wstring& error);
};

} // namespace zero
