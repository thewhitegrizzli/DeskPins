#include "stdafx.h"
#include "options.h"
#include "optionsdlg.h"


WNDPROC OptionsDlg::orgOptPSProc = 0;


void OptionsDlg::buildOptPropSheet(PROPSHEETHEADER& psh, PROPSHEETPAGE psp[], 
    int dlgIds[], DLGPROC dlgProcs[], int pageCnt, HWND parentWnd, 
    OptionsPropSheetData& data, Util::Res::ResStr& capStr)
{
    for (int n = 0; n < pageCnt; ++n) {
        psp[n].dwSize      = sizeof(psp[n]);
        psp[n].dwFlags     = PSP_HASHELP;
        psp[n].hInstance   = app.resMod ? app.resMod : app.inst;
        psp[n].pszTemplate = MAKEINTRESOURCE(dlgIds[n]);
        psp[n].hIcon       = 0;
        psp[n].pszTitle    = 0;
        psp[n].pfnDlgProc  = dlgProcs[n];
        psp[n].lParam      = reinterpret_cast<LPARAM>(&data);
        psp[n].pfnCallback = 0;
        psp[n].pcRefParent = 0;
    }

    psh.dwSize      = sizeof(psh);
    psh.dwFlags     = PSH_HASHELP | PSH_PROPSHEETPAGE | PSH_USECALLBACK | PSH_USEHICON;
    psh.hwndParent  = parentWnd;
    psh.hInstance   = app.resMod ? app.resMod : app.inst;
    psh.hIcon       = app.smIcon;
    psh.pszCaption  = capStr;
    psh.nPages      = pageCnt;
    psh.nStartPage  = (app.optionPage >= 0 && app.optionPage < pageCnt) 
        ? app.optionPage : 0;
    psh.ppsp        = psp;
    psh.pfnCallback = optPSCallback;

}


void OptionsDlg::fixOptPSPos(HWND wnd)
{
    // - find taskbar ("Shell_TrayWnd")
    // - find notification area ("TrayNotifyWnd") (child of taskbar)
    // - get center of notification area
    // - determine quadrant of screen which includes the center point
    // - position prop sheet at proper corner of workarea
    RECT rc, rcWA, rcNW;
    HWND trayWnd, notityWnd;
    if (GetWindowRect(wnd, &rc)
        && SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWA, 0)
        && (trayWnd = FindWindow(L"Shell_TrayWnd", L""))
        && (notityWnd = FindWindowEx(trayWnd, 0, L"TrayNotifyWnd", L""))
        && GetWindowRect(notityWnd, &rcNW))
    {
        // '/2' simplified from the following two inequalities
        bool isLeft = (rcNW.left + rcNW.right) < GetSystemMetrics(SM_CXSCREEN);
        bool isTop  = (rcNW.top + rcNW.bottom) < GetSystemMetrics(SM_CYSCREEN);
        int x = isLeft ? rcWA.left : rcWA.right - (rc.right - rc.left);
        int y = isTop ? rcWA.top : rcWA.bottom - (rc.bottom - rc.top);
        OffsetRect(&rc, x-rc.left, y-rc.top);
        Util::Wnd::moveWindow(wnd, rc);
    }

}


LRESULT CALLBACK OptionsDlg::optPSSubclass(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_SHOWWINDOW) {
        fixOptPSPos(wnd);

        // also set the big icon (for Alt-Tab)
        SendMessage(wnd, WM_SETICON, ICON_BIG, 
            LPARAM(LoadIcon(app.inst, MAKEINTRESOURCE(IDI_APP))));

        LRESULT ret = CallWindowProc(orgOptPSProc, wnd, msg, wparam, lparam);
        SetWindowLong(wnd, GWL_WNDPROC, LONG(orgOptPSProc));
        orgOptPSProc = 0;
        return ret;
    }

    return CallWindowProc(orgOptPSProc, wnd, msg, wparam, lparam);
}


// remove WS_EX_CONTEXTHELP and subclass to fix pos
int CALLBACK OptionsDlg::optPSCallback(HWND wnd, UINT msg, LPARAM param)
{
    if (msg == PSCB_INITIALIZED) {
        // remove caption help button
        ef::Win::WndH(wnd).modifyExStyle(WS_EX_CONTEXTHELP, 0);
        orgOptPSProc = WNDPROC(SetWindowLong(wnd, GWL_WNDPROC, LONG(optPSSubclass)));
    }

    return 0;
}
