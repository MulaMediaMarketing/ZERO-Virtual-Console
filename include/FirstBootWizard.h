#pragma once
#include "FirstBootService.h"
#include <windows.h>
#include <string>

namespace zero {

class FirstBootWizard {
public:
    FirstBootWizard(HINSTANCE instance, FirstBootService& service);
    bool Run();

private:
    enum class Step { Welcome, Profile, Controller, Display, Audio, Complete };

    HINSTANCE instance_{};
    HWND hwnd_{};
    FirstBootService& service_;
    FirstBootState state_{};
    Step step_{Step::Welcome};
    bool finished_{false};
    bool accepted_{false};
    bool controllerConnected_{false};
    unsigned volume_{80};
    WORD previousButtons_{0};
    std::wstring notice_;

    static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT HandleMessage(UINT, WPARAM, LPARAM);
    bool InitWindow();
    void EnterFullscreen();
    void Paint();
    void PollController();
    void Advance(bool fromController = false);
    void Back();
    void Complete();
    void UpdateDisplayMetadata();
    void DrawCentered(HDC dc, const std::wstring& text, int y, int height, HFONT font, COLORREF color);
};

} // namespace zero
