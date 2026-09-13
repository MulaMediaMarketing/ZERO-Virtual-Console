#pragma once
#include <string>
#include <filesystem>

namespace zero {
struct GameManifest {
    int schemaVersion{1};
    int minimumRuntimeMajor{4};
    std::string packageId;
    std::string title;
    std::string version;
    std::filesystem::path executable;
    std::filesystem::path root;
    std::filesystem::path heroImage;
    std::filesystem::path iconImage;
    std::filesystem::path logoImage;
    bool zeroResume{false};
    bool zeroAchievements{false};
    bool zeroOverlay{true};
    bool zeroInput{true};
};

enum class RuntimeState {
    Idle,
    Launching,
    Running,
    Exited,
    Crashed,
    Failed
};
}
