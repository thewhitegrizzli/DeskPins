#pragma once


// Hotkeys options tab.
//
class OptHotKeys
{
public:
    static BOOL CALLBACK dlgProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);

protected:
    static bool validate(HWND wnd);
    static void apply(HWND wnd);

    static bool changeHotkey(HWND wnd, 
        const HotKey& newHotkey, bool newState, 
        const HotKey& oldHotkey, bool oldState);

    static bool evInitDialog(HWND wnd, HWND focus, LPARAM param);

    static bool cmHotkeysOn(HWND wnd);
};
