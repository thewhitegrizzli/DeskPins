#pragma once


// Options dialog.
// Uses a property sheet to host the options tabs.
//
class OptionsDlg {
public:
    static void buildOptPropSheet(PROPSHEETHEADER& psh, PROPSHEETPAGE psp[], 
        int dlgIds[], DLGPROC dlgProcs[], int pageCnt, HWND parentWnd, 
        OptionsPropSheetData& data, Util::Res::ResStr& capStr);

private:
    static WNDPROC orgOptPSProc;

    static void fixOptPSPos(HWND wnd);
    static LRESULT CALLBACK optPSSubclass(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);
    static int CALLBACK optPSCallback(HWND wnd, UINT msg, LPARAM param);
};
