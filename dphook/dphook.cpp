// dphook.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

HINSTANCE inst;

// shared data
#pragma data_seg(".shared")
HHOOK hook      = 0;
HWND  wndEvSink = 0;
int   msg        = 0;
#pragma data_seg()

#pragma comment(linker, "/SECTION:.shared,RWS")


BOOL APIENTRY DllMain(HINSTANCE dll, DWORD reason, LPVOID)
{
    inst = dll;
    return TRUE;
}


LRESULT CALLBACK ShellProc(int code, WPARAM wparam, LPARAM lparam)
{
    if (code == HSHELL_WINDOWCREATED) {
        PostMessage(wndEvSink, msg, wparam, 0);
    }
    return CallNextHookEx(hook, code, wparam, lparam);
}


extern "C" {
    __declspec(dllexport) bool init(HWND wnd, int msgId);
    __declspec(dllexport) bool term();
}


__declspec(dllexport) bool init(HWND wnd, int msgId)
{
    hook      = SetWindowsHookEx(WH_SHELL, ShellProc, inst, 0);
    wndEvSink = wnd;
    msg       = msgId;
    return hook;
}


__declspec(dllexport) bool term()
{
    bool ok = UnhookWindowsHookEx(hook);
    hook      = 0;
    wndEvSink = 0;
    msg        = 0;
    return ok;
}
