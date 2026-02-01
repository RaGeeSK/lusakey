#pragma once

#include <windows.h>

namespace theme {
    static const COLORREF kBg = RGB(45, 45, 45);
    static const COLORREF kPanel = RGB(58, 58, 58);
    static const COLORREF kPanelAlt = RGB(68, 68, 68);
    static const COLORREF kText = RGB(235, 235, 235);
    static const COLORREF kMuted = RGB(170, 170, 170);
    static const COLORREF kOrange = RGB(255, 138, 0);
    static const COLORREF kOrangeDark = RGB(220, 118, 0);
    static const COLORREF kBorder = RGB(85, 85, 85);

    inline int DpiY() {
        HDC dc = GetDC(nullptr);
        int dpi = GetDeviceCaps(dc, LOGPIXELSY);
        ReleaseDC(nullptr, dc);
        return dpi;
    }

    inline HFONT TitleFont(int size = 20, bool semibold = true) {
        return CreateFontW(
            -MulDiv(size, DpiY(), 72),
            0, 0, 0, semibold ? FW_SEMIBOLD : FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
        );
    }

    inline HFONT BodyFont(int size = 12, bool semibold = false) {
        return CreateFontW(
            -MulDiv(size, DpiY(), 72),
            0, 0, 0, semibold ? FW_SEMIBOLD : FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
        );
    }
}
