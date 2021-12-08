#pragma once


// Tracking window for placing a pin on a target.
// It's a transparent, 1x1 window that simply captures the mouse
// and responds to left/right clicking.
// It has 'layer' in its name, because in previous versions
// it used to cover the whole screen.
//
class PinLayerWnd
{
public:
    static ATOM registerClass();
    static LRESULT CALLBACK proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);
    static LPCWSTR className;

protected:
    static bool gotInitLButtonDown;

    static void evLButtonDown(HWND wnd, UINT mk, int x, int y);
};
