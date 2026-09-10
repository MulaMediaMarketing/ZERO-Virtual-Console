#include <windows.h>
#include <filesystem>
#include <fstream>
#include <string>

int wmain() {
    wchar_t savePath[32768]{};
    DWORD n = GetEnvironmentVariableW(L"ZERO_SAVE_ROOT", savePath, 32768);
    if (n > 0) {
        std::filesystem::path p(savePath);
        std::filesystem::create_directories(p);
        std::ofstream f(p / "test_save.txt", std::ios::app);
        f << "ZERO runtime session launched successfully.\n";
    }
    MessageBoxW(nullptr,
        L"This is the ZERO runtime validation test executable.\n\nClose this window to return to ZERO.",
        L"ZERO Test Game", MB_OK | MB_ICONINFORMATION);
    return 0;
}
