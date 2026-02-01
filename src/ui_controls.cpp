#include "ui_controls.h"
#include "theme.h"

#include <gdiplus.h>
#include <unordered_map>
#include <cmath>

using namespace Gdiplus;

namespace {
    ULONG_PTR g_gdiplusToken = 0;

    struct BtnState {
        bool hover = false;
        bool accent = true;
        float t = 0.0f;
        std::wstring text;
    };

    std::unordered_map<HWND, BtnState> g_btns;

    Color ToColor(COLORREF c, BYTE a = 255) {
        return Color(a, GetRValue(c), GetGValue(c), GetBValue(c));
    }

    LRESULT CALLBACK ButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        auto it = g_btns.find(hwnd);
        if (it == g_btns.end()) {
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        BtnState& st = it->second;
        switch (msg) {
        case WM_NCDESTROY:
            g_btns.erase(hwnd);
            break;
        case WM_MOUSEMOVE: {
            if (!st.hover) {
                st.hover = true;
                TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hwnd, 0 };
                TrackMouseEvent(&tme);
                SetTimer(hwnd, 1, 16, nullptr);
            }
            break;
        }
        case WM_MOUSELEAVE:
            st.hover = false;
            SetTimer(hwnd, 1, 16, nullptr);
            break;
        case WM_TIMER: {
            float target = st.hover ? 1.0f : 0.0f;
            float delta = (target - st.t) * 0.2f;
            if (fabs(delta) < 0.01f) {
                st.t = target;
                KillTimer(hwnd, 1);
            } else {
                st.t += delta;
            }
            InvalidateRect(hwnd, nullptr, TRUE);
            break;
        }
        case WM_LBUTTONUP: {
            HWND target = GetAncestor(hwnd, GA_ROOT);
            if (!target) target = GetParent(hwnd);
            SendMessageW(target, WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), BN_CLICKED), (LPARAM)hwnd);
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);

            HDC mem = CreateCompatibleDC(hdc);
            HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
            HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

            Graphics g(mem);
            g.SetSmoothingMode(SmoothingModeAntiAlias);

            Color bg = ToColor(st.accent ? theme::kOrange : theme::kPanelAlt);
            Color bgHover = ToColor(st.accent ? theme::kOrangeDark : theme::kBorder);
            BYTE a = 255;

            int r = 12;
            RectF rect(0.0f, 0.0f, (REAL)(rc.right - rc.left), (REAL)(rc.bottom - rc.top));
            GraphicsPath path;
            path.AddArc(rect.X, rect.Y, r * 2.0f, r * 2.0f, 180, 90);
            path.AddArc(rect.GetRight() - r * 2.0f, rect.Y, r * 2.0f, r * 2.0f, 270, 90);
            path.AddArc(rect.GetRight() - r * 2.0f, rect.GetBottom() - r * 2.0f, r * 2.0f, r * 2.0f, 0, 90);
            path.AddArc(rect.X, rect.GetBottom() - r * 2.0f, r * 2.0f, r * 2.0f, 90, 90);
            path.CloseFigure();

            auto lerp = [](BYTE a, BYTE b, float t) {
                return (BYTE)(a + (b - a) * t);
            };
            Color fill(
                a,
                lerp(bg.GetR(), bgHover.GetR(), st.t),
                lerp(bg.GetG(), bgHover.GetG(), st.t),
                lerp(bg.GetB(), bgHover.GetB(), st.t)
            );
            SolidBrush brush(fill);
            g.FillPath(&brush, &path);

            SolidBrush textBrush(ToColor(theme::kText));
            Font font(L"Segoe UI", 11, FontStyleBold);
            StringFormat sf;
            sf.SetAlignment(StringAlignmentCenter);
            sf.SetLineAlignment(StringAlignmentCenter);
            RectF textRect = rect;
            g.DrawString(st.text.c_str(), (INT)st.text.size(), &font, textRect, &sf, &textBrush);

            BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, mem, 0, 0, SRCCOPY);
            SelectObject(mem, old);
            DeleteObject(bmp);
            DeleteDC(mem);
            EndPaint(hwnd, &ps);
            return 0;
        }
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    ATOM EnsureButtonClass() {
        static ATOM atom = 0;
        if (atom) return atom;
        WNDCLASSW wc{};
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"LusaButton";
        wc.lpfnWndProc = ButtonProc;
        wc.hCursor = LoadCursorW(nullptr, IDC_HAND);
        atom = RegisterClassW(&wc);
        return atom;
    }
}

namespace ui {
    void InitGdiPlus() {
        if (!g_gdiplusToken) {
            GdiplusStartupInput input;
            GdiplusStartup(&g_gdiplusToken, &input, nullptr);
        }
    }

    void ShutdownGdiPlus() {
        if (g_gdiplusToken) {
            GdiplusShutdown(g_gdiplusToken);
            g_gdiplusToken = 0;
        }
    }

    HWND CreateRoundedButton(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
        EnsureButtonClass();
        HWND hwnd = CreateWindowExW(
            0, L"LusaButton", L"",
            WS_CHILD | WS_VISIBLE,
            x, y, w, h, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr
        );
        g_btns[hwnd] = BtnState{ false, true, 0.0f, text };
        return hwnd;
    }

    void SetButtonAccent(HWND hwnd, bool accent) {
        auto it = g_btns.find(hwnd);
        if (it != g_btns.end()) {
            it->second.accent = accent;
            InvalidateRect(hwnd, nullptr, TRUE);
        }
    }
}
