#pragma once
#include "ZeroTypes.h"
#include <vector>
#include <filesystem>

namespace zero {
class GameRegistry {
public:
    explicit GameRegistry(std::filesystem::path libraryRoot);
    void Refresh();
    const std::vector<GameManifest>& Games() const noexcept { return games_; }
    const std::filesystem::path& Root() const noexcept { return root_; }
private:
    std::filesystem::path root_;
    std::vector<GameManifest> games_;
};
}
