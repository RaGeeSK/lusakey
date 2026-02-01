#pragma once

#include <windows.h>
#include <string>
#include "vault.h"

class MainWindow {
public:
    bool Create();
    int Run();

private:
    HWND hwnd_ = nullptr;
    HWND loginPage_ = nullptr;
    HWND homePage_ = nullptr;
    HWND generatorPage_ = nullptr;
    HWND settingsPage_ = nullptr;

    HWND navVault_ = nullptr;
    HWND navGen_ = nullptr;
    HWND navSettings_ = nullptr;

    HWND editMaster_ = nullptr;
    HWND btnLogin_ = nullptr;
    HWND lblLogin_ = nullptr;

    HWND listVault_ = nullptr;
    HWND lblTitle_ = nullptr;
    HWND lblCategory_ = nullptr;
    HWND lblUser_ = nullptr;
    HWND lblPass_ = nullptr;
    HWND lblUrl_ = nullptr;
    HWND lblNotes_ = nullptr;
    HWND editTitle_ = nullptr;
    HWND editCategory_ = nullptr;
    HWND editUser_ = nullptr;
    HWND editPass_ = nullptr;
    HWND editUrl_ = nullptr;
    HWND editNotes_ = nullptr;
    HWND btnAdd_ = nullptr;
    HWND btnDelete_ = nullptr;
    HWND btnSaveEntry_ = nullptr;
    HWND btnCopyUser_ = nullptr;
    HWND btnCopyPass_ = nullptr;
    HWND btnOpenUrl_ = nullptr;
    HWND btnAutofill_ = nullptr;

    HWND searchBox_ = nullptr;
    HWND filterCategory_ = nullptr;
    HWND btnImport_ = nullptr;
    HWND btnExport_ = nullptr;

    HWND genLength_ = nullptr;
    HWND genLower_ = nullptr;
    HWND genUpper_ = nullptr;
    HWND genDigits_ = nullptr;
    HWND genSymbols_ = nullptr;
    HWND genOut_ = nullptr;
    HWND genBtn_ = nullptr;
    HWND genCopy_ = nullptr;

    HWND setOld_ = nullptr;
    HWND setNew_ = nullptr;
    HWND setBtn_ = nullptr;

    std::wstring master_;
    Vault vault_;
    std::wstring filterText_;
    std::wstring filterCat_;

    int navIndicatorY_ = 140;
    int navTargetY_ = 140;

    HWND currentPage_ = nullptr;
    HWND animFrom_ = nullptr;
    HWND animTo_ = nullptr;
    int animOffset_ = 0;
    int animDir_ = 1;
    bool animating_ = false;

    void BuildUI();
    void BuildLoginPage();
    void BuildHomePage();
    void BuildGeneratorPage();
    void BuildSettingsPage();
    void ShowPage(HWND page);
    void UpdateVaultList();
    void UpdateCategoryFilters();
    void LayoutHomePage(int w, int h);
    void LoadSelection();
    void ClearEntryFields();
    void SaveEntry();
    void DeleteEntry();
    void GeneratePassword();
    void CopyToClipboard(const std::wstring& text);
    void StartPageTransition(HWND page, int dir);
    void TickPageTransition();
    void ImportCSV();
    void ExportCSV();
    void OpenUrlFromField();
    void AutofillPlaceholder();
    void AnimateNav();

    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK PageProc(HWND, UINT, WPARAM, LPARAM);
};
