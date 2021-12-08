#pragma once


// Language options tab.
//
class OptLang
{
public:
    static BOOL CALLBACK dlgProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);

protected:
    // Combobox item data.
    //
    struct Data {
        std::wstring fname, dispName, descr;
        Data(const std::wstring& fname   = L"", 
            const std::wstring& dispName = L"", 
            const std::wstring& descr    = L"")
            : fname(fname), dispName(dispName), descr(descr) {}
    };

    static bool validate(HWND wnd);
    static void apply(HWND wnd);

    static std::wstring getComboSel(HWND combo);
    static void loadLangFiles(HWND combo, const std::wstring& path, const std::wstring& cur);
    static void loadHelpFiles(HWND combo, const std::wstring& path, const std::wstring& cur);

    static bool evInitDialog(HWND wnd, HWND focus, LPARAM param);
};
