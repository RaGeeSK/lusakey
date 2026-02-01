#include "app.h"

#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif
#include "ui_controls.h"
#include "theme.h"
#include "password_gen.h"

#include <commctrl.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <commdlg.h>
#include <algorithm>
#include <fstream>
#include <locale>
#include <codecvt>
#include <vector>

#pragma comment(lib, "comctl32.lib")

namespace {
    const int kNavWidth = 220;
    const int kTopPad = 24;
    const int kPad = 16;

    enum {
        ID_NAV_VAULT = 100,
        ID_NAV_GEN = 101,
        ID_NAV_SETTINGS = 102,
        ID_LOGIN = 200,
        ID_VAULT_LIST = 300,
        ID_ADD = 301,
        ID_DELETE = 302,
        ID_SAVE_ENTRY = 303,
        ID_COPY_USER = 304,
        ID_COPY_PASS = 305,
        ID_OPEN_URL = 306,
        ID_SEARCH = 307,
        ID_FILTER = 308,
        ID_IMPORT = 309,
        ID_EXPORT = 310,
        ID_AUTOFILL = 311,
        ID_GEN = 400,
        ID_COPY = 401,
        ID_SET = 500
    };

    HFONT g_title = nullptr;
    HFONT g_body = nullptr;
    HBRUSH g_bg = nullptr;
    HBRUSH g_panel = nullptr;
    HBRUSH g_panelAlt = nullptr;
    WNDPROC g_editProc = nullptr;

    void ApplyFont(HWND h, HFONT f) {
        SendMessageW(h, WM_SETFONT, (WPARAM)f, TRUE);
    }

    LRESULT CALLBACK MasterEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
            HWND root = GetAncestor(hwnd, GA_ROOT);
            if (root) {
                SendMessageW(root, WM_COMMAND, MAKEWPARAM(ID_LOGIN, BN_CLICKED), 0);
                return 0;
            }
        }
        return CallWindowProcW(g_editProc, hwnd, msg, wParam, lParam);
    }

    std::wstring ToLower(const std::wstring& s) {
        std::wstring out = s;
        std::transform(out.begin(), out.end(), out.begin(), towlower);
        return out;
    }

    bool ContainsI(const std::wstring& hay, const std::wstring& needle) {
        if (needle.empty()) return true;
        std::wstring h = ToLower(hay);
        std::wstring n = ToLower(needle);
        return h.find(n) != std::wstring::npos;
    }

    int GetSelectedEntryIndex(HWND list) {
        int sel = ListView_GetNextItem(list, -1, LVNI_SELECTED);
        if (sel < 0) return -1;
        LVITEMW item{};
        item.iItem = sel;
        item.mask = LVIF_PARAM;
        if (ListView_GetItem(list, &item)) {
            return (int)item.lParam;
        }
        return -1;
    }

    std::vector<std::wstring> CsvSplit(const std::wstring& line) {
        std::vector<std::wstring> out;
        std::wstring cur;
        bool inQuotes = false;
        for (size_t i = 0; i < line.size(); ++i) {
            wchar_t c = line[i];
            if (c == L'"') {
                if (inQuotes && i + 1 < line.size() && line[i + 1] == L'"') {
                    cur.push_back(L'"');
                    ++i;
                } else {
                    inQuotes = !inQuotes;
                }
            } else if (c == L',' && !inQuotes) {
                out.push_back(cur);
                cur.clear();
            } else {
                cur.push_back(c);
            }
        }
        out.push_back(cur);
        return out;
    }

    std::wstring CsvEscape(const std::wstring& s) {
        bool need = s.find_first_of(L",\"\n") != std::wstring::npos;
        if (!need) return s;
        std::wstring out = L"\"";
        for (wchar_t c : s) {
            if (c == L'"') out += L"\"\"";
            else out.push_back(c);
        }
        out += L"\"";
        return out;
    }
}

bool MainWindow::Create() {
    ui::InitGdiPlus();

    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSW wc{};
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"LusaKeyMain";
    wc.lpfnWndProc = MainWindow::WndProc;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    WNDCLASSW pc{};
    pc.hInstance = GetModuleHandleW(nullptr);
    pc.lpszClassName = L"LusaPage";
    pc.lpfnWndProc = MainWindow::PageProc;
    pc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&pc);

    hwnd_ = CreateWindowExW(
        0, L"LusaKeyMain", L"LusaKey",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 720,
        nullptr, nullptr, GetModuleHandleW(nullptr), this
    );
    return hwnd_ != nullptr;
}

int MainWindow::Run() {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    ui::ShutdownGdiPlus();
    return (int)msg.wParam;
}

void MainWindow::BuildUI() {
    g_title = theme::TitleFont(22, true);
    g_body = theme::BodyFont(12, false);
    g_bg = CreateSolidBrush(theme::kBg);
    g_panel = CreateSolidBrush(theme::kPanel);
    g_panelAlt = CreateSolidBrush(theme::kPanelAlt);

    BuildLoginPage();
    BuildHomePage();
    BuildGeneratorPage();
    BuildSettingsPage();

    navVault_ = ui::CreateRoundedButton(hwnd_, ID_NAV_VAULT, L"Хранилище", 24, 140, 170, 36);
    navGen_ = ui::CreateRoundedButton(hwnd_, ID_NAV_GEN, L"Генератор", 24, 190, 170, 36);
    navSettings_ = ui::CreateRoundedButton(hwnd_, ID_NAV_SETTINGS, L"Настройки", 24, 240, 170, 36);
    ui::SetButtonAccent(navGen_, false);
    ui::SetButtonAccent(navSettings_, false);

    ShowPage(loginPage_);
}

void MainWindow::BuildLoginPage() {
    loginPage_ = CreateWindowExW(0, L"LusaPage", L"", WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, hwnd_, nullptr, GetModuleHandleW(nullptr), nullptr);

    lblLogin_ = CreateWindowExW(0, L"STATIC", L"Добро пожаловать в LusaKey",
        WS_CHILD | WS_VISIBLE, kNavWidth + 60, 120, 400, 40,
        loginPage_, nullptr, GetModuleHandleW(nullptr), nullptr);
    ApplyFont(lblLogin_, g_title);

    CreateWindowExW(0, L"STATIC", L"Мастер‑пароль",
        WS_CHILD | WS_VISIBLE, kNavWidth + 60, 190, 300, 24,
        loginPage_, nullptr, GetModuleHandleW(nullptr), nullptr);

    editMaster_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_PASSWORD, kNavWidth + 60, 220, 360, 34,
        loginPage_, nullptr, GetModuleHandleW(nullptr), nullptr);
    ApplyFont(editMaster_, g_body);
    g_editProc = (WNDPROC)SetWindowLongPtrW(editMaster_, GWLP_WNDPROC, (LONG_PTR)MasterEditProc);

    btnLogin_ = ui::CreateRoundedButton(loginPage_, ID_LOGIN, L"Войти / Создать хранилище", kNavWidth + 60, 280, 260, 42);

    ApplyFont(btnLogin_, g_body);
}

void MainWindow::BuildHomePage() {
    homePage_ = CreateWindowExW(0, L"LusaPage", L"", WS_CHILD,
        0, 0, 0, 0, hwnd_, nullptr, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", L"Хранилище",
        WS_CHILD | WS_VISIBLE, kNavWidth + 40, kTopPad, 300, 28,
        homePage_, nullptr, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", L"Поиск",
        WS_CHILD | WS_VISIBLE, kNavWidth + 40, 52, 80, 18,
        homePage_, nullptr, GetModuleHandleW(nullptr), nullptr);
    searchBox_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE, kNavWidth + 40, 70, 220, 26,
        homePage_, (HMENU)ID_SEARCH, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", L"Категория",
        WS_CHILD | WS_VISIBLE, kNavWidth + 270, 52, 120, 18,
        homePage_, nullptr, GetModuleHandleW(nullptr), nullptr);
    filterCategory_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, kNavWidth + 270, 70, 170, 200,
        homePage_, (HMENU)ID_FILTER, GetModuleHandleW(nullptr), nullptr);

    btnImport_ = ui::CreateRoundedButton(homePage_, ID_IMPORT, L"Импорт CSV", kNavWidth + 460, 66, 130, 32);
    btnExport_ = ui::CreateRoundedButton(homePage_, ID_EXPORT, L"Экспорт CSV", kNavWidth + 600, 66, 130, 32);

    listVault_ = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL, kNavWidth + 40, 110, 500, 480,
        homePage_, (HMENU)ID_VAULT_LIST, GetModuleHandleW(nullptr), nullptr);
    ApplyFont(listVault_, g_body);

    ListView_SetExtendedListViewStyle(listVault_, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    ListView_SetBkColor(listVault_, theme::kPanel);
    ListView_SetTextBkColor(listVault_, theme::kPanel);
    ListView_SetTextColor(listVault_, theme::kText);
    LVCOLUMNW col{};
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.pszText = (LPWSTR)L"Название";
    col.cx = 150;
    ListView_InsertColumn(listVault_, 0, &col);
    col.pszText = (LPWSTR)L"Категория";
    col.cx = 110;
    ListView_InsertColumn(listVault_, 1, &col);
    col.pszText = (LPWSTR)L"Логин";
    col.cx = 140;
    ListView_InsertColumn(listVault_, 2, &col);
    col.pszText = (LPWSTR)L"Сайт";
    col.cx = 140;
    ListView_InsertColumn(listVault_, 3, &col);

    lblTitle_ = CreateWindowExW(0, L"STATIC", L"Название", WS_CHILD | WS_VISIBLE,
        760, 90, 160, 20, homePage_, nullptr, GetModuleHandleW(nullptr), nullptr);
    editTitle_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE, 760, 110, 260, 28,
        homePage_, nullptr, GetModuleHandleW(nullptr), nullptr);

    lblCategory_ = CreateWindowExW(0, L"STATIC", L"Категория", WS_CHILD | WS_VISIBLE,
        760, 150, 160, 20, homePage_, nullptr, GetModuleHandleW(nullptr), nullptr);
    editCategory_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWN, 760, 170, 260, 200,
        homePage_, nullptr, GetModuleHandleW(nullptr), nullptr);

    lblUser_ = CreateWindowExW(0, L"STATIC", L"Логин", WS_CHILD | WS_VISIBLE,
        760, 210, 160, 20, homePage_, nullptr, GetModuleHandleW(nullptr), nullptr);
    editUser_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE, 760, 230, 260, 28,
        homePage_, nullptr, GetModuleHandleW(nullptr), nullptr);

    lblPass_ = CreateWindowExW(0, L"STATIC", L"Пароль", WS_CHILD | WS_VISIBLE,
        760, 270, 160, 20, homePage_, nullptr, GetModuleHandleW(nullptr), nullptr);
    editPass_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_PASSWORD, 760, 290, 260, 28,
        homePage_, nullptr, GetModuleHandleW(nullptr), nullptr);

    lblUrl_ = CreateWindowExW(0, L"STATIC", L"Сайт", WS_CHILD | WS_VISIBLE,
        760, 330, 160, 20, homePage_, nullptr, GetModuleHandleW(nullptr), nullptr);
    editUrl_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE, 760, 350, 260, 28,
        homePage_, nullptr, GetModuleHandleW(nullptr), nullptr);

    lblNotes_ = CreateWindowExW(0, L"STATIC", L"Заметки", WS_CHILD | WS_VISIBLE,
        760, 390, 160, 20, homePage_, nullptr, GetModuleHandleW(nullptr), nullptr);
    editNotes_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE, 760, 410, 260, 90,
        homePage_, nullptr, GetModuleHandleW(nullptr), nullptr);

    btnCopyUser_ = ui::CreateRoundedButton(homePage_, ID_COPY_USER, L"Копировать логин", 760, 510, 120, 34);
    btnCopyPass_ = ui::CreateRoundedButton(homePage_, ID_COPY_PASS, L"Копировать пароль", 900, 510, 120, 34);

    btnOpenUrl_ = ui::CreateRoundedButton(homePage_, ID_OPEN_URL, L"Открыть сайт", 760, 550, 260, 34);

    btnAutofill_ = ui::CreateRoundedButton(homePage_, ID_AUTOFILL, L"Автозаполнение (beta)", 760, 590, 260, 34);

    btnAdd_ = ui::CreateRoundedButton(homePage_, ID_ADD, L"Добавить", 760, 630, 120, 36);
    btnDelete_ = ui::CreateRoundedButton(homePage_, ID_DELETE, L"Удалить", 900, 630, 120, 36);

    btnSaveEntry_ = ui::CreateRoundedButton(homePage_, ID_SAVE_ENTRY, L"Сохранить", 760, 674, 260, 40);

    ApplyFont(searchBox_, g_body);
    ApplyFont(filterCategory_, g_body);
    ApplyFont(editTitle_, g_body);
    ApplyFont(editCategory_, g_body);
    ApplyFont(editUser_, g_body);
    ApplyFont(editPass_, g_body);
    ApplyFont(editUrl_, g_body);
    ApplyFont(editNotes_, g_body);

    RECT rc;
    GetClientRect(hwnd_, &rc);
    LayoutHomePage(rc.right, rc.bottom);
}

void MainWindow::BuildGeneratorPage() {
    generatorPage_ = CreateWindowExW(0, L"LusaPage", L"", WS_CHILD,
        0, 0, 0, 0, hwnd_, nullptr, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", L"Генератор паролей",
        WS_CHILD | WS_VISIBLE, kNavWidth + 40, kTopPad, 360, 28,
        generatorPage_, nullptr, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", L"Длина", WS_CHILD | WS_VISIBLE,
        kNavWidth + 40, 90, 120, 20, generatorPage_, nullptr, GetModuleHandleW(nullptr), nullptr);
    genLength_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"16",
        WS_CHILD | WS_VISIBLE, kNavWidth + 40, 112, 80, 28,
        generatorPage_, nullptr, GetModuleHandleW(nullptr), nullptr);

    genLower_ = CreateWindowExW(0, L"BUTTON", L"Строчные", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        kNavWidth + 40, 160, 140, 24, generatorPage_, nullptr, GetModuleHandleW(nullptr), nullptr);
    genUpper_ = CreateWindowExW(0, L"BUTTON", L"Прописные", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        kNavWidth + 40, 190, 140, 24, generatorPage_, nullptr, GetModuleHandleW(nullptr), nullptr);
    genDigits_ = CreateWindowExW(0, L"BUTTON", L"Цифры", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        kNavWidth + 40, 220, 140, 24, generatorPage_, nullptr, GetModuleHandleW(nullptr), nullptr);
    genSymbols_ = CreateWindowExW(0, L"BUTTON", L"Символы", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        kNavWidth + 40, 250, 140, 24, generatorPage_, nullptr, GetModuleHandleW(nullptr), nullptr);
    SendMessageW(genLower_, BM_SETCHECK, BST_CHECKED, 0);
    SendMessageW(genUpper_, BM_SETCHECK, BST_CHECKED, 0);
    SendMessageW(genDigits_, BM_SETCHECK, BST_CHECKED, 0);

    genOut_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_READONLY, kNavWidth + 40, 300, 360, 32,
        generatorPage_, nullptr, GetModuleHandleW(nullptr), nullptr);

    genBtn_ = ui::CreateRoundedButton(generatorPage_, ID_GEN, L"Сгенерировать", kNavWidth + 40, 350, 140, 38);
    genCopy_ = ui::CreateRoundedButton(generatorPage_, ID_COPY, L"Копировать", kNavWidth + 200, 350, 140, 38);
    ui::SetButtonAccent(genCopy_, false);

    ApplyFont(genLength_, g_body);
    ApplyFont(genLower_, g_body);
    ApplyFont(genUpper_, g_body);
    ApplyFont(genDigits_, g_body);
    ApplyFont(genSymbols_, g_body);
    ApplyFont(genOut_, g_body);
}

void MainWindow::BuildSettingsPage() {
    settingsPage_ = CreateWindowExW(0, L"LusaPage", L"", WS_CHILD,
        0, 0, 0, 0, hwnd_, nullptr, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", L"Смена мастер‑пароля",
        WS_CHILD | WS_VISIBLE, kNavWidth + 40, kTopPad, 360, 28,
        settingsPage_, nullptr, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", L"Текущий", WS_CHILD | WS_VISIBLE,
        kNavWidth + 40, 90, 120, 20, settingsPage_, nullptr, GetModuleHandleW(nullptr), nullptr);
    setOld_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_PASSWORD, kNavWidth + 40, 112, 260, 28,
        settingsPage_, nullptr, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", L"Новый", WS_CHILD | WS_VISIBLE,
        kNavWidth + 40, 152, 120, 20, settingsPage_, nullptr, GetModuleHandleW(nullptr), nullptr);
    setNew_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_PASSWORD, kNavWidth + 40, 174, 260, 28,
        settingsPage_, nullptr, GetModuleHandleW(nullptr), nullptr);

    setBtn_ = ui::CreateRoundedButton(settingsPage_, ID_SET, L"Обновить мастер‑пароль", kNavWidth + 40, 220, 260, 40);

    ApplyFont(setOld_, g_body);
    ApplyFont(setNew_, g_body);
}

void MainWindow::ShowPage(HWND page) {
    ShowWindow(loginPage_, page == loginPage_ ? SW_SHOW : SW_HIDE);
    ShowWindow(homePage_, page == homePage_ ? SW_SHOW : SW_HIDE);
    ShowWindow(generatorPage_, page == generatorPage_ ? SW_SHOW : SW_HIDE);
    ShowWindow(settingsPage_, page == settingsPage_ ? SW_SHOW : SW_HIDE);

    bool showNav = page != loginPage_;
    ShowWindow(navVault_, showNav ? SW_SHOW : SW_HIDE);
    ShowWindow(navGen_, showNav ? SW_SHOW : SW_HIDE);
    ShowWindow(navSettings_, showNav ? SW_SHOW : SW_HIDE);
    currentPage_ = page;
}

void MainWindow::UpdateVaultList() {
    ListView_DeleteAllItems(listVault_);
    int row = 0;
    for (size_t i = 0; i < vault_.entries.size(); ++i) {
        const auto& e = vault_.entries[i];
        if (!filterCat_.empty() && filterCat_ != L"Все" && e.category != filterCat_) {
            continue;
        }
        if (!filterText_.empty()) {
            if (!ContainsI(e.title, filterText_) &&
                !ContainsI(e.username, filterText_) &&
                !ContainsI(e.url, filterText_) &&
                !ContainsI(e.notes, filterText_) &&
                !ContainsI(e.category, filterText_)) {
                continue;
            }
        }
        LVITEMW item{};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = row;
        item.lParam = (LPARAM)i;
        item.pszText = (LPWSTR)e.title.c_str();
        int inserted = ListView_InsertItem(listVault_, &item);
        if (inserted >= 0) row = inserted;
        ListView_SetItemText(listVault_, row, 1, (LPWSTR)e.category.c_str());
        ListView_SetItemText(listVault_, row, 2, (LPWSTR)e.username.c_str());
        ListView_SetItemText(listVault_, row, 3, (LPWSTR)e.url.c_str());
        row++;
    }
}

void MainWindow::UpdateCategoryFilters() {
    SendMessageW(filterCategory_, CB_RESETCONTENT, 0, 0);
    SendMessageW(filterCategory_, CB_ADDSTRING, 0, (LPARAM)L"Все");

    std::vector<std::wstring> cats;
    for (const auto& e : vault_.entries) {
        if (!e.category.empty()) cats.push_back(e.category);
    }
    std::sort(cats.begin(), cats.end());
    cats.erase(std::unique(cats.begin(), cats.end()), cats.end());
    for (const auto& c : cats) {
        SendMessageW(filterCategory_, CB_ADDSTRING, 0, (LPARAM)c.c_str());
    }
    int sel = 0;
    if (!filterCat_.empty() && filterCat_ != L"Все") {
        for (size_t i = 0; i < cats.size(); ++i) {
            if (cats[i] == filterCat_) {
                sel = (int)i + 1;
                break;
            }
        }
    } else {
        filterCat_ = L"Все";
    }
    SendMessageW(filterCategory_, CB_SETCURSEL, sel, 0);

    SendMessageW(editCategory_, CB_RESETCONTENT, 0, 0);
    for (const auto& c : cats) {
        SendMessageW(editCategory_, CB_ADDSTRING, 0, (LPARAM)c.c_str());
    }
}

void MainWindow::LayoutHomePage(int w, int h) {
    const int pad = 40;
    const int rightW = 260;
    const int rightX = w - pad - rightW;
    const int leftX = kNavWidth + pad;
    const int topY = 110;
    const int listW = std::max(300, rightX - 20 - leftX);
    const int listH = std::max(240, h - topY - 60);

    MoveWindow(searchBox_, leftX, 70, 220, 26, TRUE);
    MoveWindow(filterCategory_, leftX + 230, 70, 170, 200, TRUE);
    MoveWindow(btnImport_, leftX + 420, 66, 130, 32, TRUE);
    MoveWindow(btnExport_, leftX + 560, 66, 130, 32, TRUE);

    MoveWindow(listVault_, leftX, topY, listW, listH, TRUE);

    int y = 90;
    MoveWindow(lblTitle_, rightX, y, rightW, 20, TRUE);
    y += 20;
    MoveWindow(editTitle_, rightX, y, rightW, 28, TRUE);
    y += 40;
    MoveWindow(lblCategory_, rightX, y, rightW, 20, TRUE);
    y += 20;
    MoveWindow(editCategory_, rightX, y, rightW, 28, TRUE);
    y += 40;
    MoveWindow(lblUser_, rightX, y, rightW, 20, TRUE);
    y += 20;
    MoveWindow(editUser_, rightX, y, rightW, 28, TRUE);
    y += 40;
    MoveWindow(lblPass_, rightX, y, rightW, 20, TRUE);
    y += 20;
    MoveWindow(editPass_, rightX, y, rightW, 28, TRUE);
    y += 40;
    MoveWindow(lblUrl_, rightX, y, rightW, 20, TRUE);
    y += 20;
    MoveWindow(editUrl_, rightX, y, rightW, 28, TRUE);
    y += 40;
    MoveWindow(lblNotes_, rightX, y, rightW, 20, TRUE);
    y += 20;
    MoveWindow(editNotes_, rightX, y, rightW, 90, TRUE);
    y += 110;
    MoveWindow(btnCopyUser_, rightX, y, (rightW / 2) - 5, 34, TRUE);
    MoveWindow(btnCopyPass_, rightX + (rightW / 2) + 5, y, (rightW / 2) - 5, 34, TRUE);
    y += 40;
    MoveWindow(btnOpenUrl_, rightX, y, rightW, 34, TRUE);
    y += 40;
    MoveWindow(btnAutofill_, rightX, y, rightW, 34, TRUE);
    y += 44;
    MoveWindow(btnAdd_, rightX, y, (rightW / 2) - 5, 36, TRUE);
    MoveWindow(btnDelete_, rightX + (rightW / 2) + 5, y, (rightW / 2) - 5, 36, TRUE);
    y += 44;
    MoveWindow(btnSaveEntry_, rightX, y, rightW, 40, TRUE);
}

void MainWindow::LoadSelection() {
    int idx = GetSelectedEntryIndex(listVault_);
    if (idx < 0 || idx >= (int)vault_.entries.size()) return;
    const auto& e = vault_.entries[idx];
    SetWindowTextW(editTitle_, e.title.c_str());
    SetWindowTextW(editCategory_, e.category.c_str());
    SetWindowTextW(editUser_, e.username.c_str());
    SetWindowTextW(editPass_, e.password.c_str());
    SetWindowTextW(editUrl_, e.url.c_str());
    SetWindowTextW(editNotes_, e.notes.c_str());
}

void MainWindow::ClearEntryFields() {
    SetWindowTextW(editTitle_, L"");
    SetWindowTextW(editCategory_, L"");
    SetWindowTextW(editUser_, L"");
    SetWindowTextW(editPass_, L"");
    SetWindowTextW(editUrl_, L"");
    SetWindowTextW(editNotes_, L"");
}

void MainWindow::SaveEntry() {
    wchar_t buf[512];
    Entry e;
    GetWindowTextW(editTitle_, buf, 512); e.title = buf;
    GetWindowTextW(editCategory_, buf, 512); e.category = buf;
    GetWindowTextW(editUser_, buf, 512); e.username = buf;
    GetWindowTextW(editPass_, buf, 512); e.password = buf;
    GetWindowTextW(editUrl_, buf, 512); e.url = buf;
    GetWindowTextW(editNotes_, buf, 512); e.notes = buf;

    int idx = GetSelectedEntryIndex(listVault_);
    if (idx >= 0 && idx < (int)vault_.entries.size()) {
        vault_.entries[idx] = e;
    } else {
        vault_.entries.push_back(e);
    }
    vault::Save(master_, vault_);
    UpdateCategoryFilters();
    UpdateVaultList();
    ClearEntryFields();
}

void MainWindow::DeleteEntry() {
    int idx = GetSelectedEntryIndex(listVault_);
    if (idx < 0 || idx >= (int)vault_.entries.size()) return;
    vault_.entries.erase(vault_.entries.begin() + idx);
    vault::Save(master_, vault_);
    UpdateCategoryFilters();
    UpdateVaultList();
    ClearEntryFields();
}

void MainWindow::GeneratePassword() {
    wchar_t buf[32];
    GetWindowTextW(genLength_, buf, 32);
    int len = _wtoi(buf);
    bool lower = SendMessageW(genLower_, BM_GETCHECK, 0, 0) == BST_CHECKED;
    bool upper = SendMessageW(genUpper_, BM_GETCHECK, 0, 0) == BST_CHECKED;
    bool digits = SendMessageW(genDigits_, BM_GETCHECK, 0, 0) == BST_CHECKED;
    bool symbols = SendMessageW(genSymbols_, BM_GETCHECK, 0, 0) == BST_CHECKED;
    std::wstring out = passgen::Generate(len, lower, upper, digits, symbols);
    SetWindowTextW(genOut_, out.c_str());
}

void MainWindow::CopyToClipboard(const std::wstring& text) {
    if (!OpenClipboard(hwnd_)) return;
    EmptyClipboard();
    size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (hmem) {
        void* ptr = GlobalLock(hmem);
        memcpy(ptr, text.c_str(), bytes);
        GlobalUnlock(hmem);
        SetClipboardData(CF_UNICODETEXT, hmem);
    }
    CloseClipboard();
}

void MainWindow::OpenUrlFromField() {
    wchar_t buf[512];
    GetWindowTextW(editUrl_, buf, 512);
    std::wstring url = buf;
    if (url.empty()) return;
    ShellExecuteW(hwnd_, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void MainWindow::AutofillPlaceholder() {
    wchar_t user[256], pass[256], url[512];
    GetWindowTextW(editUser_, user, 256);
    GetWindowTextW(editPass_, pass, 256);
    GetWindowTextW(editUrl_, url, 512);
    if (wcslen(pass) == 0) return;
    CopyToClipboard(pass);
    if (wcslen(url) > 0) {
        ShellExecuteW(hwnd_, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
    }
    MessageBoxW(hwnd_, L"Автозаполнение (beta): пароль скопирован.\nОткройте сайт и вставьте вручную.",
        L"LusaKey", MB_OK | MB_ICONINFORMATION);
}

void MainWindow::StartPageTransition(HWND page, int dir) {
    if (currentPage_ == page || animating_) {
        ShowPage(page);
        return;
    }
    RECT rc;
    GetClientRect(hwnd_, &rc);
    animFrom_ = currentPage_;
    animTo_ = page;
    animDir_ = dir;
    animOffset_ = 0;
    animating_ = true;

    ShowWindow(animTo_, SW_SHOW);
    SetWindowPos(animTo_, nullptr, animDir_ * rc.right, 0, rc.right, rc.bottom, SWP_NOZORDER);
    SetTimer(hwnd_, 3, 16, nullptr);
}

void MainWindow::TickPageTransition() {
    if (!animating_) return;
    RECT rc;
    GetClientRect(hwnd_, &rc);
    int step = rc.right / 18;
    animOffset_ += step;
    if (animOffset_ >= rc.right) {
        animating_ = false;
        KillTimer(hwnd_, 3);
        ShowWindow(animFrom_, SW_HIDE);
        SetWindowPos(animTo_, nullptr, 0, 0, rc.right, rc.bottom, SWP_NOZORDER);
        currentPage_ = animTo_;
        return;
    }
    int xFrom = -animDir_ * animOffset_;
    int xTo = animDir_ * (rc.right - animOffset_);
    SetWindowPos(animFrom_, nullptr, xFrom, 0, rc.right, rc.bottom, SWP_NOZORDER);
    SetWindowPos(animTo_, nullptr, xTo, 0, rc.right, rc.bottom, SWP_NOZORDER);
}

void MainWindow::ImportCSV() {
    wchar_t filePath[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = L"CSV Files\0*.csv\0All Files\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return;

    std::wifstream file(filePath);
    file.imbue(std::locale(std::locale::classic(), new std::codecvt_utf8<wchar_t>));
    if (!file) return;

    std::wstring line;
    bool first = true;
    while (std::getline(file, line)) {
        if (first) {
            first = false;
            continue;
        }
        auto cols = CsvSplit(line);
        if (cols.size() < 5) continue;
        Entry e;
        e.title = cols.size() > 0 ? cols[0] : L"";
        e.category = cols.size() > 1 ? cols[1] : L"";
        e.username = cols.size() > 2 ? cols[2] : L"";
        e.password = cols.size() > 3 ? cols[3] : L"";
        e.url = cols.size() > 4 ? cols[4] : L"";
        e.notes = cols.size() > 5 ? cols[5] : L"";
        vault_.entries.push_back(e);
    }
    vault::Save(master_, vault_);
    UpdateCategoryFilters();
    UpdateVaultList();
}

void MainWindow::ExportCSV() {
    wchar_t filePath[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = L"CSV Files\0*.csv\0All Files\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"csv";
    if (!GetSaveFileNameW(&ofn)) return;

    std::wofstream file(filePath);
    file.imbue(std::locale(std::locale::classic(), new std::codecvt_utf8<wchar_t>));
    if (!file) return;

    file << L"title,category,username,password,url,notes\n";
    for (const auto& e : vault_.entries) {
        file << CsvEscape(e.title) << L","
            << CsvEscape(e.category) << L","
            << CsvEscape(e.username) << L","
            << CsvEscape(e.password) << L","
            << CsvEscape(e.url) << L","
            << CsvEscape(e.notes) << L"\n";
    }
}

void MainWindow::AnimateNav() {
    int delta = navTargetY_ - navIndicatorY_;
    if (abs(delta) < 2) {
        navIndicatorY_ = navTargetY_;
        KillTimer(hwnd_, 2);
    } else {
        navIndicatorY_ += delta / 5;
    }
    InvalidateRect(hwnd_, nullptr, TRUE);
}

LRESULT CALLBACK MainWindow::PageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN: {
        HDC dc = (HDC)wParam;
        SetTextColor(dc, theme::kText);
        SetBkColor(dc, theme::kPanel);
        return (LRESULT)g_panel;
    }
    case WM_ERASEBKGND: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        HBRUSH br = CreateSolidBrush(theme::kBg);
        FillRect((HDC)wParam, &rc, br);
        DeleteObject(br);
        return 1;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
        self = (MainWindow*)cs->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)self);
        self->hwnd_ = hwnd;
    } else {
        self = (MainWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    }
    if (!self) return DefWindowProcW(hwnd, msg, wParam, lParam);

    switch (msg) {
    case WM_CREATE: {
        self->BuildUI();
        const int corner = 2; // DWMWCP_ROUND
        DwmSetWindowAttribute(hwnd, 33, &corner, sizeof(corner));
        return 0;
    }
    case WM_SIZE: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        MoveWindow(self->loginPage_, 0, 0, rc.right, rc.bottom, TRUE);
        MoveWindow(self->homePage_, 0, 0, rc.right, rc.bottom, TRUE);
        MoveWindow(self->generatorPage_, 0, 0, rc.right, rc.bottom, TRUE);
        MoveWindow(self->settingsPage_, 0, 0, rc.right, rc.bottom, TRUE);
        self->LayoutHomePage(rc.right, rc.bottom);
        return 0;
    }
    case WM_TIMER:
        if (wParam == 2) self->AnimateNav();
        if (wParam == 3) self->TickPageTransition();
        return 0;
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_LOGIN) {
            wchar_t buf[128];
            GetWindowTextW(self->editMaster_, buf, 128);
            self->master_ = buf;
            Vault v;
            if (vault::Load(self->master_, v)) {
                self->vault_ = v;
            } else {
                self->vault_.entries.clear();
                vault::Save(self->master_, self->vault_);
            }
            self->filterText_.clear();
            SetWindowTextW(self->searchBox_, L"");
            self->UpdateCategoryFilters();
            self->UpdateVaultList();
            self->StartPageTransition(self->homePage_, 1);
            self->navTargetY_ = 140;
            ui::SetButtonAccent(self->navVault_, true);
            ui::SetButtonAccent(self->navGen_, false);
            ui::SetButtonAccent(self->navSettings_, false);
            SetTimer(hwnd, 2, 16, nullptr);
        } else if (id == ID_NAV_VAULT) {
            int dir = self->navTargetY_ < 140 ? -1 : 1;
            self->StartPageTransition(self->homePage_, dir);
            self->navTargetY_ = 140;
            ui::SetButtonAccent(self->navVault_, true);
            ui::SetButtonAccent(self->navGen_, false);
            ui::SetButtonAccent(self->navSettings_, false);
            SetTimer(hwnd, 2, 16, nullptr);
        } else if (id == ID_NAV_GEN) {
            int dir = self->navTargetY_ < 190 ? -1 : 1;
            self->StartPageTransition(self->generatorPage_, dir);
            self->navTargetY_ = 190;
            ui::SetButtonAccent(self->navVault_, false);
            ui::SetButtonAccent(self->navGen_, true);
            ui::SetButtonAccent(self->navSettings_, false);
            SetTimer(hwnd, 2, 16, nullptr);
        } else if (id == ID_NAV_SETTINGS) {
            int dir = self->navTargetY_ < 240 ? -1 : 1;
            self->StartPageTransition(self->settingsPage_, dir);
            self->navTargetY_ = 240;
            ui::SetButtonAccent(self->navVault_, false);
            ui::SetButtonAccent(self->navGen_, false);
            ui::SetButtonAccent(self->navSettings_, true);
            SetTimer(hwnd, 2, 16, nullptr);
        } else if (id == ID_ADD) {
            self->ClearEntryFields();
        } else if (id == ID_SAVE_ENTRY) {
            self->SaveEntry();
        } else if (id == ID_DELETE) {
            self->DeleteEntry();
        } else if (id == ID_COPY_USER) {
            wchar_t buf[256];
            GetWindowTextW(self->editUser_, buf, 256);
            self->CopyToClipboard(buf);
        } else if (id == ID_COPY_PASS) {
            wchar_t buf[256];
            GetWindowTextW(self->editPass_, buf, 256);
            self->CopyToClipboard(buf);
        } else if (id == ID_OPEN_URL) {
            self->OpenUrlFromField();
        } else if (id == ID_AUTOFILL) {
            self->AutofillPlaceholder();
        } else if (id == ID_IMPORT) {
            self->ImportCSV();
        } else if (id == ID_EXPORT) {
            self->ExportCSV();
        } else if (id == ID_GEN) {
            self->GeneratePassword();
        } else if (id == ID_COPY) {
            wchar_t buf[256];
            GetWindowTextW(self->genOut_, buf, 256);
            self->CopyToClipboard(buf);
        } else if (id == ID_SET) {
            wchar_t oldp[128], newp[128];
            GetWindowTextW(self->setOld_, oldp, 128);
            GetWindowTextW(self->setNew_, newp, 128);
            Vault v;
            if (vault::Load(oldp, v)) {
                self->master_ = newp;
                self->vault_ = v;
                vault::Save(self->master_, self->vault_);
                self->UpdateCategoryFilters();
                self->UpdateVaultList();
                SetWindowTextW(self->setOld_, L"");
                SetWindowTextW(self->setNew_, L"");
            }
        } else if (id == ID_SEARCH && HIWORD(wParam) == EN_CHANGE) {
            wchar_t buf[256];
            GetWindowTextW(self->searchBox_, buf, 256);
            self->filterText_ = buf;
            self->UpdateVaultList();
        } else if (id == ID_FILTER && HIWORD(wParam) == CBN_SELCHANGE) {
            int sel = (int)SendMessageW(self->filterCategory_, CB_GETCURSEL, 0, 0);
            if (sel >= 0) {
                wchar_t buf[256];
                SendMessageW(self->filterCategory_, CB_GETLBTEXT, sel, (LPARAM)buf);
                self->filterCat_ = buf;
                self->UpdateVaultList();
            }
        }
        return 0;
    }
    case WM_NOTIFY: {
        LPNMHDR hdr = (LPNMHDR)lParam;
        if (hdr->idFrom == ID_VAULT_LIST) {
            if (hdr->code == LVN_ITEMCHANGED) {
                self->LoadSelection();
            } else if (hdr->code == NM_CUSTOMDRAW) {
                LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)lParam;
                if (cd->nmcd.dwDrawStage == CDDS_PREPAINT) {
                    return CDRF_NOTIFYITEMDRAW;
                }
                if (cd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                    cd->clrText = theme::kText;
                    cd->clrTextBk = theme::kPanel;
                    return CDRF_DODEFAULT;
                }
            }
        }
        if (hdr->hwndFrom == ListView_GetHeader(self->listVault_) && hdr->code == NM_CUSTOMDRAW) {
            LPNMCUSTOMDRAW cd = (LPNMCUSTOMDRAW)lParam;
            if (cd->dwDrawStage == CDDS_PREPAINT) {
                return CDRF_NOTIFYITEMDRAW;
            }
            if (cd->dwDrawStage == CDDS_ITEMPREPAINT) {
                HDC dc = cd->hdc;
                RECT rc = cd->rc;
                HBRUSH br = CreateSolidBrush(theme::kPanelAlt);
                FillRect(dc, &rc, br);
                DeleteObject(br);
                SetTextColor(dc, theme::kText);
                SetBkMode(dc, TRANSPARENT);
                return CDRF_DODEFAULT;
            }
        }
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN: {
        HDC dc = (HDC)wParam;
        SetTextColor(dc, theme::kText);
        SetBkColor(dc, theme::kPanel);
        return (LRESULT)g_panel;
    }
    case WM_ERASEBKGND: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, g_bg);
        return 1;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        HBRUSH navBrush = CreateSolidBrush(theme::kPanelAlt);
        RECT nav = { 0, 0, kNavWidth, rc.bottom };
        FillRect(hdc, &nav, navBrush);
        DeleteObject(navBrush);

        HBRUSH indicator = CreateSolidBrush(theme::kOrange);
        RECT ind = { 8, self->navIndicatorY_, 12, self->navIndicatorY_ + 36 };
        FillRect(hdc, &ind, indicator);
        DeleteObject(indicator);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
