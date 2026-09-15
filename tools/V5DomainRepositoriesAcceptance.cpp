#include "v5/DomainRepositories.h"
#include "ZeroTypes.h"
#include <filesystem>
#include <iostream>

using namespace zero::v5;

int main() {
    const auto root = std::filesystem::temp_directory_path() / L"zero-v5-domain-repositories";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    if (ec) return 10;

    LocalDomainDatabase local(root / L"platform.db");
    std::string error;
    if (!local.Initialize(error)) return 11;

    zero::GameManifest game;
    game.packageId = "pkg.v5.repositories";
    game.title = "Repository Fixture";
    game.version = "1.0.0";
    game.executable = root / L"fixture.exe";
    game.root = root;
    std::wstring wideError;
    if (!local.Database()->UpsertGame(game, wideError)) return 12;

    LocalSettingsRepository settings(local.Database());
    if (!settings.Set("v5.theme", "dark", error)) return 20;
    const auto setting = settings.Get("v5.theme", error);
    if (!setting || *setting != "dark") return 21;

    LocalResumeRepository resume(local.Database());
    zero::ResumeMetadata metadata{game.packageId, "checkpoint-1", "Checkpoint One", "payload", "2026-09-15T00:00:00Z"};
    if (!resume.Save(metadata, error)) return 30;
    const auto loadedResume = resume.Load(game.packageId, error);
    if (!loadedResume || loadedResume->activityId != "checkpoint-1") return 31;
    if (!resume.Clear(game.packageId, error)) return 32;
    if (resume.Load(game.packageId, error)) return 33;

    LocalAchievementRepository achievements(local.Database());
    if (!achievements.Save(game.packageId, "ach-1", "First Step", "2026-09-15T00:00:00Z", error)) return 40;
    if (achievements.Load(game.packageId, error).size() != 1) return 41;

    LocalSessionRepository sessions(local.Database());
    if (!sessions.Record({game.packageId, "session-1", 60, 0, "clean_exit", false}, error)) return 50;

    LocalSettingsRepository missing(nullptr);
    if (missing.Set("x", "y", error)) return 60;

    std::filesystem::remove_all(root, ec);
    std::cout << "ZERO V5 persistence/domain repository acceptance: PASS\n";
    return 0;
}
