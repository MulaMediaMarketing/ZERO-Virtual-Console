#include "App.h"
#include <windows.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    zero::App app(hInstance);
    return app.Run();
}
