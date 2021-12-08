#pragma once


// General options tab.
//
class OptPins
{
public:
    static BOOL CALLBACK dlgProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);

protected:
    static bool validate(HWND wnd);
    static void apply(HWND wnd);

    static HBRUSH getPinClrBrush(HWND wnd);
    static COLORREF getPinClr(HWND wnd);
    static void setPinClrBrush(HWND wnd, COLORREF clr);

    static BOOL CALLBACK enumWndProcPinsUpdate(HWND wnd, LPARAM);
    static void updatePinWnds();
    static BOOL CALLBACK resetPinTimersEnumProc(HWND wnd, LPARAM param);

    static bool evInitDialog(HWND wnd, HWND focus, LPARAM lparam);
    static void evTermDialog(HWND wnd);
    static bool evDrawItem(HWND wnd, UINT id, DRAWITEMSTRUCT* dis);

    static void cmPinClr(HWND wnd);
};
