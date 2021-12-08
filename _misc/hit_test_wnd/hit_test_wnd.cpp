#include "stdafx.h"
using ef::tchar;
using ef::Win::Point;

static HINSTANCE inst;

const UINT WM_APP_TRACK = WM_APP + 0;
enum { TRACK_MOVE, TRACK_SELECT, TRACK_CANCEL };


LRESULT CALLBACK LayerWndProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCT& cs = *(CREATESTRUCT*)lparam;
            SetWindowLong(wnd, GWL_USERDATA, (LONG)cs.lpCreateParams);
            return 0;
        }
        case WM_MOUSEMOVE: {
            Point pt = Point::Packed(lparam);
            ClientToScreen(wnd, &pt);
            HWND listener = (HWND)GetWindowLong(wnd, GWL_USERDATA);
            PostMessage(listener, WM_APP_TRACK, TRACK_MOVE, pt.packed());
            return 0;
        }
        case WM_LBUTTONDOWN: {
            Point pt = Point::Packed(lparam);
            ClientToScreen(wnd, &pt);
            HWND listener = (HWND)GetWindowLong(wnd, GWL_USERDATA);
            PostMessage(listener, WM_APP_TRACK, TRACK_SELECT, pt.packed());
            return 0;
        }
        case WM_RBUTTONDOWN: {
            HWND listener = (HWND)GetWindowLong(wnd, GWL_USERDATA);
            PostMessage(listener, WM_APP_TRACK, TRACK_CANCEL, 0);
            return 0;
        }
    }
    return DefWindowProc(wnd, msg, wparam, lparam);
}


class MouseTracker : boost::noncopyable
{
    HWND layerWnd1, layerWnd2;

public:
    MouseTracker() {}
    ~MouseTracker() {}

    bool isActive() const {
        return layerWnd1 && layerWnd2;
    }

    bool start(HWND listener) {
        // FIXME: multiple monitors
        int w = GetSystemMetrics(SM_CXSCREEN);
        int h = GetSystemMetrics(SM_CYSCREEN);

        DWORD style = WS_POPUP | WS_VISIBLE;
        DWORD exStyle = WS_EX_TOPMOST | WS_EX_TRANSPARENT;

        // NOTE: If we create a single window covering the whole screen,
        // the taskbar is automatically made non-topmost and sent to the bottom of the z-order.
        // This can be observed if there's a maximized window (the top border of the taskbar is hidden)
        // or any normal window partially obscured by the taskbar.
        // The solution is to create more than one windows, each covering part of the screen.

        // NOTE: The parent must be 0, otherwise (e.g. if it's this wnd) the taskbar
        // will be moved on top of our topmost windows.

        layerWnd1 = CreateWindowEx(exStyle, _T("LayerWnd"), _T("layer1"),
            style, 0, 0, w/2, h, 0, 0, inst, (LPVOID)listener);
        layerWnd2 = CreateWindowEx(exStyle, _T("LayerWnd"), _T("layer2"),
            style, w/2, 0, w - w/2, h, 0, 0, inst, (LPVOID)listener);

        if (!layerWnd1 || !layerWnd2) {
            if (layerWnd1) DestroyWindow(layerWnd1);
            if (layerWnd2) DestroyWindow(layerWnd2);
            return false;
        }

        SetFocus(layerWnd1);
        SetFocus(layerWnd2);
        return true;
    }

    void stop() {
        if (layerWnd1) DestroyWindow(layerWnd1);
        if (layerWnd2) DestroyWindow(layerWnd2);
        layerWnd1 = layerWnd2 = 0;
    }
};


LRESULT CALLBACK MainWndProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    static MouseTracker tracker;
    static tchar statusMsgBuf[100];

    const tchar* msgFmt_tracking = _T("tracking: %d x %d");
    const tchar* msgFmt_selected = _T("last selected: %d x %d");
    const tchar* msgFmt_notSelected = _T("last selected: none");

    switch (msg) {
        case WM_CREATE: {
            wsprintf(statusMsgBuf, msgFmt_notSelected);
            return 0;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            if (!tracker.start(wnd))
                MessageBox(wnd, _T("Could not start mouse tracker."), 0, 0);
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(wnd, &ps);
            const tchar* prompt = tracker.isActive() ? _T("Left-click to select, right-click to cancel.") : _T("Click here to start tracking.");
            TextOut(dc, 0, 0, prompt, lstrlen(prompt));
            TextOut(dc, 0, 20, statusMsgBuf, lstrlen(statusMsgBuf));
            EndPaint(wnd, &ps);
            return 0;
        }
        case WM_APP_TRACK: {
            Point pt = Point::Packed(lparam);
            switch (wparam) {
            case TRACK_MOVE:
                wsprintf(statusMsgBuf, msgFmt_tracking, pt.x, pt.y);
                InvalidateRect(wnd, 0, true);
                break;
            case TRACK_SELECT:
                wsprintf(statusMsgBuf, msgFmt_selected, pt.x, pt.y);
                tracker.stop();
                InvalidateRect(wnd, 0, true);
                break;
            case TRACK_CANCEL:
                wsprintf(statusMsgBuf, msgFmt_notSelected);
                tracker.stop();
                InvalidateRect(wnd, 0, true);
                break;
            }
            return 0;
        }
    }

    return DefWindowProc(wnd, msg, wparam, lparam);
}


bool RegWndCls()
{
    WNDCLASS wc;

    wc.style         = 0;
    wc.lpfnWndProc   = MainWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = inst;
    wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = HBRUSH(COLOR_WINDOW + 1);
    wc.lpszMenuName  = 0;
    wc.lpszClassName = _T("MainWnd");
    if (!RegisterClass(&wc)) return false;

    wc.style         = 0;
    wc.lpfnWndProc   = LayerWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = inst;
    wc.hIcon         = 0;
    wc.hCursor       = LoadCursor(0, IDC_HAND);
    wc.hbrBackground = 0; //HBRUSH(COLOR_3DFACE + 1);
    wc.lpszMenuName  = 0;
    wc.lpszClassName = _T("LayerWnd");
    if (!RegisterClass(&wc)) return false;

    return true;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR szCmdLine, int cmdShow)
{
    inst = hInstance;

    if (!RegWndCls())
        return 0;

    HWND mainWnd = CreateWindowEx(0, _T("MainWnd"), _T("Mouse tracking test"),
        WS_OVERLAPPEDWINDOW,
        100, 100, 300, 100, 
        0, 0, inst, 0);

    ShowWindow(mainWnd, cmdShow);
    UpdateWindow(mainWnd);

    MSG msg;
    while (GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}
