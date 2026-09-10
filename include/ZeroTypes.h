#pragma once
#include <string>
#include <filesystem>

namespace zero {
struct GameManifest {
    std::string packageId;
    std::string title;
    std::string version;
    std::filesystem::path executable;
    std::filesystem::path root;
    std::filesystem::path heroImage;
    std::filesystem::path iconImage;
    bool zeroResume{false};
    bool zeroAchievements{false};
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
