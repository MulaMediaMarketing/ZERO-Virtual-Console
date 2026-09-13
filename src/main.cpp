#include "App.h"
#include "FirstBootService.h"
#include "FirstBootWizard.h"
#include <windows.h>
#include <shlobj.h>
#include <filesystem>

namespace {
std::filesystem::path zeroRoot() {
    PWSTR p = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &p))) {
        std::filesystem::path root = std::filesystem::path(p) / "ZERO";
        CoTaskMemFree(p);
        return root;
    }
    return std::filesystem::current_path() / "ZeroData";
}
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    zero::FirstBootService firstBoot(zeroRoot());
    if (firstBoot.IsRequired()) {
        zero::FirstBootWizard wizard(hInstance, firstBoot);
        if (!wizard.Run()) return 0;
    }

    zero::App app(hInstance);
    return app.Run();
}
