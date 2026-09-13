#include "GameImportService.h"
#include "GameRegistry.h"
#include "ManifestValidator.h"
#include <fstream>
#include <sstream>

namespace zero {
namespace {
std::string readAll(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return {};
    std::ostringstream ss; ss << f.rdbuf(); return ss.str();
}
std::string jsonString(const std::string& s, const std::string& key) {
    const std::string token = "\"" + key + "\"";
    auto k = s.find(token); if (k == std::string::npos) return {};
    auto c = s.find(':', k + token.size()); if (c == std::string::npos) return {};
    auto q1 = s.find('"', c + 1); if (q1 == std::string::npos) return {};
    auto q2 = s.find('"', q1 + 1); if (q2 == std::string::npos) return {};
    return s.substr(q1 + 1, q2 - q1 - 1);
}
}

GameImportService::GameImportService(std::filesystem::path libraryRoot)
    : libraryRoot_(std::move(libraryRoot)) {}

ImportResult GameImportService::ImportFolder(const std::filesystem::path& sourceRoot) const {
    ImportResult result;
    const auto manifest = sourceRoot / "zero.manifest.json";
    if (!std::filesystem::exists(manifest)) {
        result.error = L"The selected folder does not contain zero.manifest.json.";
        return result;
    }

    const auto text = readAll(manifest);
    const auto packageId = jsonString(text, "package_id");
    if (!ManifestValidator::IsSafePackageId(packageId)) {
        result.error = L"The package_id is missing or invalid.";
        return result;
    }

    std::error_code ec;
    std::filesystem::create_directories(libraryRoot_, ec);
    if (ec) { result.error = L"ZERO could not access the Library directory."; return result; }

    const auto stagingBase = libraryRoot_ / L".staging";
    const auto staging = stagingBase / std::filesystem::path(packageId.begin(), packageId.end());
    const auto destination = libraryRoot_ / std::filesystem::path(packageId.begin(), packageId.end());
    std::filesystem::remove_all(staging, ec); ec.clear();
    std::filesystem::create_directories(staging, ec);
    if (ec) { result.error = L"ZERO could not create the import staging directory."; return result; }

    std::filesystem::copy(sourceRoot, staging,
        std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        std::filesystem::remove_all(staging, ec);
        result.error = L"ZERO could not stage the selected game folder.";
        return result;
    }

    GameRegistry stagedRegistry(stagingBase);
    stagedRegistry.Refresh();
    const auto& games = stagedRegistry.Games();
    if (games.size() != 1 || games.front().packageId != packageId) {
        std::filesystem::remove_all(staging, ec);
        result.error = L"The staged game failed ZERO manifest validation.";
        return result;
    }

    if (std::filesystem::exists(destination)) {
        std::filesystem::remove_all(staging, ec);
        result.error = L"This ZERO package is already installed. Update/replace support is handled separately.";
        return result;
    }

    std::filesystem::rename(staging, destination, ec);
    if (ec) {
        std::filesystem::remove_all(staging, ec);
        result.error = L"ZERO could not finalize the imported game.";
        return result;
    }

    result.success = true;
    result.installedRoot = destination;
    return result;
}

} // namespace zero
