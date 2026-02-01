#pragma once

#include <windows.h>
#include <string>

namespace ui {
    void InitGdiPlus();
    void ShutdownGdiPlus();

    HWND CreateRoundedButton(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h);
    void SetButtonAccent(HWND hwnd, bool accent);
}
