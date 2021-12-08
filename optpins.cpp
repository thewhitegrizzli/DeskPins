#include "stdafx.h"
#include "options.h"
#include "util.h"
#include "pinwnd.h"
#include "optpins.h"
#include "resource.h"


#define OWNERDRAW_CLR_BUTTON  0


HBRUSH OptPins::getPinClrBrush(HWND wnd)
{
    HWND ctrl = GetDlgItem(wnd, IDC_PIN_COLOR_BOX);
    return HBRUSH(GetWindowLong(ctrl, GWL_USERDATA));
}


COLORREF OptPins::getPinClr(HWND wnd)
{
    HBRUSH brush = getPinClrBrush(wnd);
    LOGBRUSH lb;
    return GetObject(brush, sizeof(LOGBRUSH), &lb) ? lb.lbColor : 0;
}


void OptPins::setPinClrBrush(HWND wnd, COLORREF clr)
{
    DeleteObject(getPinClrBrush(wnd));

    HWND ctrl = GetDlgItem(wnd, IDC_PIN_COLOR_BOX);
    SetWindowLong(ctrl, GWL_USERDATA, LONG(CreateSolidBrush(clr)));
    InvalidateRect(ctrl, 0, true);
}


bool OptPins::evInitDialog(HWND wnd, HWND focus, LPARAM lparam)
{
    // must have a valid data ptr
    if (!lparam) {
        EndDialog(wnd, IDCANCEL);
        return false;
    }

    // save the data ptr
    PROPSHEETPAGE& psp = *reinterpret_cast<PROPSHEETPAGE*>(lparam);
    Options& opt = reinterpret_cast<OptionsPropSheetData*>(psp.lParam)->opt;
    SetWindowLong(wnd, DWL_USER, psp.lParam);

#if OWNERDRAW_CLR_BUTTON
    LONG style = GetWindowLong(GetDlgItem(wnd, IDC_PIN_COLOR), GWL_STYLE);
    style |= BS_OWNERDRAW;
    SetWindowLong(GetDlgItem(wnd, IDC_PIN_COLOR), GWL_STYLE, style);
#endif

    setPinClrBrush(wnd, opt.pinClr);

    SendDlgItemMessage(wnd, IDC_POLL_RATE_UD, UDM_SETRANGE, 0, 
        MAKELONG(opt.trackRate.maxV,opt.trackRate.minV));
    SendDlgItemMessage(wnd, IDC_POLL_RATE_UD, UDM_SETPOS, 0, 
        MAKELONG(opt.trackRate.value,0));

    CheckDlgButton(wnd, 
        opt.dblClkTray ? IDC_TRAY_DOUBLE_CLICK : IDC_TRAY_SINGLE_CLICK,
        BST_CHECKED);

    if (opt.runOnStartup)
        CheckDlgButton(wnd, IDC_RUN_ON_STARTUP, BST_CHECKED);

    return false;
}


void OptPins::evTermDialog(HWND wnd)
{
    DeleteObject(getPinClrBrush(wnd));
}


#if OWNERDRAW_CLR_BUTTON

// TODO: add vistual styles support
bool OptPins::evDrawItem(HWND wnd, UINT id, DRAWITEMSTRUCT* dis)
{
    if (id != IDC_PIN_COLOR)
        return false;

    RECT rc = dis->rcItem;
    HDC dc = dis->hDC;
    bool pressed = dis->itemState & ODS_SELECTED;

    DrawFrameControl(dc, &rc, DFC_BUTTON, 
        DFCS_BUTTONPUSH | DFCS_ADJUSTRECT | (pressed ? DFCS_PUSHED : 0));

    InflateRect(&rc, -1, -1);
    if (dis->itemState & ODS_FOCUS)
        DrawFocusRect(dc, &rc);
    InflateRect(&rc, -1, -1);

    if (pressed)
        OffsetRect(&rc, 1, 1);

    int h = rc.bottom - rc.left;
    RECT rc2;
    CopyRect(&rc2, &rc);
    rc2.right = rc2.left + 2*h;
    InflateRect(&rc2, -1, -1);
    DrawEdge(dc, &rc2, BDR_SUNKENOUTER, BF_RECT | BF_ADJUST);
    COLORREF clr = (COLORREF)GetWindowLong(GetDlgItem(wnd, IDC_PIN_COLOR), GWL_USERDATA);
    HBRUSH brush = CreateSolidBrush(clr);
    FillRect(dc, &rc2, brush);
    DeleteObject(brush);

    rc.left = rc2.right + 4;
    WCHAR buf[40];
    GetDlgItemText(wnd, id, buf, sizeof(buf));
    DrawText(dc, buf, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SetWindowLong(wnd, DWL_MSGRESULT, true);
    return true;
}

#endif


bool OptPins::validate(HWND wnd)
{
    Options& opt = reinterpret_cast<OptionsPropSheetData*>(GetWindowLong(wnd, DWL_USER))->opt;
    return opt.trackRate.validateUI(wnd, IDC_POLL_RATE, true);
}


void OptPins::cmPinClr(HWND wnd)
{
    static COLORREF userClrs[16] = {0};

    CHOOSECOLOR cc;
    cc.lStructSize  = sizeof(cc);
    cc.hwndOwner    = wnd;
    cc.rgbResult    = getPinClr(wnd);
    cc.lpCustColors = userClrs;
    cc.Flags        = CC_RGBINIT | CC_SOLIDCOLOR;   //CC_ANYCOLOR
    if (ChooseColor(&cc)) {
        setPinClrBrush(wnd, cc.rgbResult);
        Util::Wnd::psChanged(wnd);
    }
}


BOOL CALLBACK OptPins::enumWndProcPinsUpdate(HWND wnd, LPARAM)
{
    if (Util::Text::strimatch(PinWnd::className, ef::Win::WndH(wnd).getClassName().c_str()))
        InvalidateRect(wnd, 0, false);
    return true;    // continue
}


void OptPins::updatePinWnds()
{
    EnumWindows((WNDENUMPROC)enumWndProcPinsUpdate, 0);
}


BOOL CALLBACK OptPins::resetPinTimersEnumProc(HWND wnd, LPARAM param)
{
    if (Util::Text::strimatch(PinWnd::className, ef::Win::WndH(wnd).getClassName().c_str()))
        SendMessage(wnd, App::WM_PIN_RESETTIMER, param, 0);
    return true;    // continue
}


void OptPins::apply(HWND wnd)
{
    Options& opt = reinterpret_cast<OptionsPropSheetData*>(GetWindowLong(wnd, DWL_USER))->opt;

    COLORREF clr = getPinClr(wnd);
    if (opt.pinClr != clr) {
        app.createSmClrIcon(opt.pinClr = clr);
        app.trayIcon.setIcon(app.smClrIcon);
        if (app.pinShape.initImage(clr)) updatePinWnds();
    }

    int rate = opt.trackRate.getUI(wnd, IDC_POLL_RATE);
    if (opt.trackRate.value != rate)
        EnumWindows(resetPinTimersEnumProc, opt.trackRate.value = rate);

    opt.dblClkTray = IsDlgButtonChecked(wnd, IDC_TRAY_DOUBLE_CLICK) == BST_CHECKED;

    opt.runOnStartup = IsDlgButtonChecked(wnd, IDC_RUN_ON_STARTUP) == BST_CHECKED;
}


BOOL CALLBACK OptPins::dlgProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{

    switch (msg) {
        case WM_INITDIALOG:  return evInitDialog(wnd, HWND(wparam), lparam);
        case WM_DESTROY:     evTermDialog(wnd); return true;
        case WM_NOTIFY: {
            NMHDR nmhdr = *reinterpret_cast<NMHDR*>(lparam);
            switch (nmhdr.code) {
                case PSN_SETACTIVE: {
                    HWND tab = PropSheet_GetTabControl(nmhdr.hwndFrom);
                    app.optionPage = SendMessage(tab, TCM_GETCURSEL, 0, 0);
                    SetWindowLong(wnd, DWL_MSGRESULT, 0);
                    return true;
                }
                case PSN_KILLACTIVE: {
                    SetWindowLong(wnd, DWL_MSGRESULT, !validate(wnd));
                    return true;
                }
                case PSN_APPLY:
                    apply(wnd);
                    return true;
                case PSN_HELP: {
                    app.help.show(wnd, L"::\\optpins.htm");
                    return true;
                }
                case UDN_DELTAPOS: {
                    if (wparam == IDC_POLL_RATE_UD) {
                        NM_UPDOWN& nmud = *(NM_UPDOWN*)lparam;
                        Options& opt = 
                            reinterpret_cast<OptionsPropSheetData*>(GetWindowLong(wnd, DWL_USER))->opt;
                        nmud.iDelta *= opt.trackRate.step;
                        SetWindowLong(wnd, DWL_MSGRESULT, FALSE);   // allow change
                        return true;
                    }
                    else
                        return false;
                }
                default:
                    return false;
            }
        }
        case WM_HELP: {
            app.help.show(wnd, L"::\\optpins.htm");
            return true;
        }
        case WM_COMMAND: {
            WORD id = LOWORD(wparam), code = HIWORD(wparam);
            switch (id) {
                case IDC_PIN_COLOR:         cmPinClr(wnd); return true;
                case IDC_PIN_COLOR_BOX:     if (code == STN_DBLCLK) cmPinClr(wnd); return true;
                case IDC_TRAY_SINGLE_CLICK:
                case IDC_TRAY_DOUBLE_CLICK: Util::Wnd::psChanged(wnd); return true;
                case IDC_POLL_RATE:         if (code == EN_CHANGE) Util::Wnd::psChanged(wnd); return true;
                default:                    return false;
            }
        }
#if OWNERDRAW_CLR_BUTTON
        case WM_DRAWITEM:
            return EvDrawItem(wnd, wparam, (DRAWITEMSTRUCT*)lparam);
#endif
        case WM_CTLCOLORSTATIC:
            if (HWND(lparam) == GetDlgItem(wnd, IDC_PIN_COLOR_BOX))
                return BOOL(getPinClrBrush(wnd));
            return 0;
        default:
            return false;
    }

}
