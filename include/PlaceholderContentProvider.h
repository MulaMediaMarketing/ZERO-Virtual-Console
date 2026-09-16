#pragma once

#include <windows.h>
#include <array>
#include <cwchar>
#include <filesystem>

namespace zero {

struct PlaceholderGameInfo {
    const wchar_t* id;
    const wchar_t* title;
    const wchar_t* developer;
    const wchar_t* genre;
    const wchar_t* description;
    const wchar_t* playState;
    const wchar_t* imageFile;
};

// TEST-ONLY PLACEHOLDER CONTENT.
// This provider exists strictly for UI preview, visual QA, clipping tests and
// responsive-layout validation. It never participates in Store, entitlement,
// achievement, download, social, cloud, commerce or package authority paths.
class PlaceholderContentProvider final {
public:
    static bool Enabled() noexcept {
        const wchar_t* commandLine = GetCommandLineW();
        return commandLine && std::wcsstr(commandLine, L"--ui-preview") != nullptr;
    }

    static std::filesystem::path AssetRoot() {
        std::array<wchar_t, 32768> modulePath{};
        const DWORD count = GetModuleFileNameW(nullptr, modulePath.data(), static_cast<DWORD>(modulePath.size()));
        if (count == 0 || count >= modulePath.size()) return {};
        return std::filesystem::path(modulePath.data()).parent_path() / L"assets" / L"placeholders";
    }

    static const std::array<PlaceholderGameInfo, 12>& Games() noexcept {
        static const std::array<PlaceholderGameInfo, 12> games{{
            {L"echoes-of-tomorrow", L"Echoes of Tomorrow", L"ZERO Originals", L"Sci-Fi Action RPG", L"A fractured world. A second chance.", L"Last played: 2 hours ago", L"covers/echoes-of-tomorrow.png"},
            {L"nexus-rising", L"Nexus Rising", L"Vector Arc Studios", L"Cyberpunk Story Action", L"A city of signals, secrets, and shifting loyalties.", L"Last played: 1 day ago", L"covers/nexus-rising.png"},
            {L"voidrunner", L"Voidrunner", L"Helix Forge", L"Space Combat", L"Cross unstable systems at impossible speed.", L"Last played: 3 days ago", L"covers/voidrunner.png"},
            {L"ashen-realms", L"Ashen Realms", L"Obsidian Crest", L"Dark Fantasy Adventure", L"Kingdoms burn. Legends remain.", L"Last played: 5 days ago", L"covers/ashen-realms.png"},
            {L"starfall-protocol", L"Starfall Protocol", L"Northstar Labs", L"Tactical Sci-Fi", L"A distant signal becomes humanity's most dangerous mission.", L"Last played: 1 week ago", L"covers/starfall-protocol.png"},
            {L"the-last-light", L"The Last Light", L"Emberline Games", L"Narrative Adventure", L"Keep the final beacon alive across a collapsing frontier.", L"Recently played", L"covers/the-last-light.png"},
            {L"drift-legends", L"Drift Legends", L"Apex Streetworks", L"Arcade Racing", L"Build speed, style, and reputation across night-city circuits.", L"Recently played", L"covers/drift-legends.png"},
            {L"silent-reach", L"Silent Reach", L"Black Harbor", L"Stealth Adventure", L"Enter the ruins where every sound changes the hunt.", L"Recently played", L"covers/silent-reach.png"},
            {L"boundless", L"Boundless", L"Wildsignal Studio", L"Exploration", L"A vast world without walls, routes, or a single correct path.", L"Recently played", L"covers/boundless.png"},
            {L"karma-dark-within", L"Karma: The Dark Within", L"Nightglass", L"Psychological Action", L"What follows you home may have always been there.", L"Recently played", L"covers/karma-dark-within.png"},
            {L"project-orion", L"Project Orion", L"Orbital House", L"Sci-Fi Adventure", L"A deep-space expedition uncovers a machine older than history.", L"Recently played", L"covers/project-orion.png"},
            {L"the-infinite-within", L"The Infinite Within", L"ZERO Originals", L"Cinematic Adventure", L"The largest worlds can hide inside a single memory.", L"ZERO Original", L"covers/the-infinite-within.png"}
        }};
        return games;
    }

    static std::filesystem::path HeroImage() {
        return AssetRoot() / L"heroes" / L"hero-echoes-of-tomorrow.png";
    }
};

} // namespace zero
