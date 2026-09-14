#include "GameRegistry.h"
#include "PackageManifestParser.h"
#include <algorithm>

namespace zero {

GameRegistry::GameRegistry(std::filesystem::path libraryRoot) : root_(std::move(libraryRoot)) {}

void GameRegistry::Refresh() {
    games_.clear();
    std::error_code ec;
    std::filesystem::create_directories(root_, ec);
    if (ec || !std::filesystem::exists(root_)) return;

    for (const auto& entry : std::filesystem::directory_iterator(root_, ec)) {
        if (ec || !entry.is_directory()) continue;
        const auto name = entry.path().filename();
        if (name == L".staging" || name == L".repair-backup") continue;

        const auto manifestPath = entry.path() / L"zero.manifest.json";
        if (!std::filesystem::exists(manifestPath, ec) || ec) {
            ec.clear();
            continue;
        }

        auto parsed = PackageManifestParser::ParseFile(manifestPath, entry.path());
        if (!parsed.valid) continue;
        games_.push_back(std::move(parsed.manifest));
    }

    std::sort(games_.begin(), games_.end(), [](const GameManifest& a, const GameManifest& b) {
        if (a.title == b.title) return a.packageId < b.packageId;
        return a.title < b.title;
    });
}

} // namespace zero
