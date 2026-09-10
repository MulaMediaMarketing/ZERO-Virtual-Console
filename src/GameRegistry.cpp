#include "GameRegistry.h"
#include <fstream>
#include <sstream>
#include <algorithm>

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

bool jsonBool(const std::string& s, const std::string& key, bool fallback) {
    const std::string token = "\"" + key + "\"";
    auto k = s.find(token); if (k == std::string::npos) return fallback;
    auto c = s.find(':', k + token.size()); if (c == std::string::npos) return fallback;
    auto tail = s.substr(c + 1, 8);
    if (tail.find("true") != std::string::npos) return true;
    if (tail.find("false") != std::string::npos) return false;
    return fallback;
}
}

GameRegistry::GameRegistry(std::filesystem::path libraryRoot) : root_(std::move(libraryRoot)) {}

void GameRegistry::Refresh() {
    games_.clear();
    std::error_code ec;
    std::filesystem::create_directories(root_, ec);
    if (!std::filesystem::exists(root_)) return;

    for (const auto& entry : std::filesystem::directory_iterator(root_, ec)) {
        if (ec || !entry.is_directory()) continue;
        auto manifestPath = entry.path() / "zero.manifest.json";
        if (!std::filesystem::exists(manifestPath)) continue;
        auto text = readAll(manifestPath);
        if (text.empty()) continue;

        GameManifest g;
        g.root = entry.path();
        g.packageId = jsonString(text, "package_id");
        g.title = jsonString(text, "title");
        g.version = jsonString(text, "version");
        g.executable = g.root / jsonString(text, "executable");
        const auto hero = jsonString(text, "hero_image");
        const auto icon = jsonString(text, "icon_image");
        if (!hero.empty()) g.heroImage = g.root / hero;
        if (!icon.empty()) g.iconImage = g.root / icon;
        g.zeroResume = jsonBool(text, "zero_resume", false);
        g.zeroAchievements = jsonBool(text, "zero_achievements", false);
        g.zeroInput = jsonBool(text, "zero_input", true);

        if (g.packageId.empty() || g.title.empty() || g.executable.empty()) continue;
        if (!std::filesystem::exists(g.executable)) continue;
        games_.push_back(std::move(g));
    }

    std::sort(games_.begin(), games_.end(), [](const GameManifest& a, const GameManifest& b){
        return a.title < b.title;
    });
}
}
