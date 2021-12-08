#pragma once


class Options;
class WindowCreationMonitor;


// Main window.
// Handles notification icon menu, hotkeys, autopin.
// It also commnunicates with the pin windows (those do the actual work
// of pinning).
//
class MainWnd
{
public:
    static ATOM registerClass();
    static LRESULT CALLBACK proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);
    static LPCWSTR className;

protected:
    static void evHotkey(HWND wnd, int idHotKey, Options* opt);
    static void evPinReq(HWND wnd, int x, int y, Options* opt);
    static void evTrayIcon(HWND wnd, WPARAM id, LPARAM msg, Options* opt);

    static void cmNewPin(HWND wnd);
    static void cmRemovePins(HWND wnd);
    static void cmOptions(HWND wnd, WindowCreationMonitor& winCreMon, Options* opt);
};
