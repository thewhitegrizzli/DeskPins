#include "stdafx.h"
#include "util.h"
#include "options.h"
#include "apputils.h"


HWINEVENTHOOK EventHookWindowCreationMonitor::hook = 0;
HWND EventHookWindowCreationMonitor::wnd = 0;
int EventHookWindowCreationMonitor::msgId = 0;


void PendingWindows::add(HWND wnd)
{
    Entry entry(wnd, GetTickCount());
    entries.push_back(entry);
}


void PendingWindows::check(HWND wnd, const Options& opt)
{
    for (int n = entries.size()-1; n >= 0; --n) {
        if (timeToChkWnd(entries[n].time, opt)) {
            if (checkWnd(entries[n].wnd, opt))
                Util::App::pinWindow(wnd, entries[n].wnd, opt.trackRate.value, true);
            entries.erase(entries.begin() + n);
        }
    }
}


bool PendingWindows::timeToChkWnd(DWORD t, const Options& opt)
{
    // ticks are unsigned, so wrap-around is ok
    return GetTickCount() - t >= DWORD(opt.autoPinDelay.value);
}


bool PendingWindows::checkWnd(HWND target, const Options& opt)
{
    for (int n = 0; n < int(opt.autoPinRules.size()); ++n)
        if (opt.autoPinRules[n].match(target))
            return true;
    return false;
}


bool EventHookWindowCreationMonitor::init(HWND wnd, int msgId)
{
    if (!hook) {
        hook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_CREATE, 
            0, proc, 0, 0, WINEVENT_OUTOFCONTEXT);
        if (hook) {
            this->wnd = wnd;
            this->msgId = msgId;
        }
    }
    return hook != 0;
}


bool EventHookWindowCreationMonitor::term()
{
    if (hook && UnhookWinEvent(hook)) {
        hook = 0;
    }
    return !hook;
}


VOID CALLBACK EventHookWindowCreationMonitor::proc(HWINEVENTHOOK hook, DWORD event,
    HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (hook == EventHookWindowCreationMonitor::hook &&
        event == EVENT_OBJECT_CREATE &&
        idObject == OBJID_WINDOW)
    {
        PostMessage(wnd, msgId, (WPARAM)hwnd, 0);
    }
}


bool HookDllWindowCreationMonitor::init(HWND wnd, int msgId)
{
    if (!dll) {
        dll = LoadLibrary(L"dphook.dll");
        if (!dll)
            return false;
    }

    dllInitFunc = initF(GetProcAddress(dll, "init"));
    dllTermFunc = termF(GetProcAddress(dll, "term"));
    if (!dllInitFunc || !dllTermFunc) {
        term();
        return false;
    }

    return dllInitFunc(wnd, msgId);
}


bool HookDllWindowCreationMonitor::term()
{
    if (!dll)
        return true;

    if (dllTermFunc)
        dllTermFunc();

    bool ok = !!FreeLibrary(dll);
    if (ok)
        dll = 0;

    return ok;
}
