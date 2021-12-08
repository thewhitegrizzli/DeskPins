#include "stdafx.h"
#include "pinshape.h"
#include "util.h"
#include "apputils.h"
#include "help.h"
#include "options.h"
#include "app.h"
#include "resource.h"

//#define TEST_OPTIONS_PAGE  1

// enable visual styles
#pragma comment(linker, "/manifestdependency:\""                               \
    "type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' " \
    "processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'" \
    "\"")

#include "ef/std/addlib.hpp"
#include "ef/Win/addlib.hpp"


// global app object
App app;

// options object visible only in this file
static Options opt;


//------------------------------------------
/*enum {
    CMDLINE_NEWPIN     = 1<<0,  // n
    CMDLINE_REMOVEALL  = 1<<1,  // r
    CMDLINE_OPTIONS    = 1<<2,  // o
    CMDLINE_QUIT       = 1<<3,  // q
    CMDLINE_HIDE       = 1<<4,  // h
    CMDLINE_SHOW       = 1<<5,  // s
    CMDLINE_AUTOEXIT   = 1<<6,  // x
    CMDLINE_NOAUTOEXIT = 1<<7,  // -x
};


typedef void (&CmdLineCallback)(int flag);


// Parse cmdline (from global VC vars), passing flags to callback.
// Invalid switches and parameters are ignored.
// Return true if any switch is processed,
// so that the process can terminate if needed.
static bool parseCmdLine(CmdLineCallback cb) {
    bool msgSent = false;
    typedef const char** Ptr;
    Ptr end = (Ptr)__argv + __argc;
    for (Ptr s = (Ptr)__argv + 1; s < end; ++s) {
        char c = *((*s)++);
        if (c == '/' || c == '-') {
            if      (_stricmp(*s, "n"))  { cb(CMDLINE_NEWPIN);     msgSent = true; }
            else if (_stricmp(*s, "r"))  { cb(CMDLINE_REMOVEALL);  msgSent = true; }
            else if (_stricmp(*s, "o"))  { cb(CMDLINE_OPTIONS);    msgSent = true; }
            else if (_stricmp(*s, "p"))  { cb(CMDLINE_QUIT);       msgSent = true; }
            else if (_stricmp(*s, "h"))  { cb(CMDLINE_HIDE);       msgSent = true; }
            else if (_stricmp(*s, "s"))  { cb(CMDLINE_SHOW);       msgSent = true; }
            else if (_stricmp(*s, "x"))  { cb(CMDLINE_AUTOEXIT);   msgSent = true; }
            else if (_stricmp(*s, "-x")) { cb(CMDLINE_NOAUTOEXIT); msgSent = true; }
        }
    }
    return msgSent;
}


struct DispatcherParams
{
    DispatcherParams(HANDLE doneEvent) :
        doneEvent(doneEvent) {}
    HANDLE doneEvent;  // 0 if not needed
};


HWND findAppWindow()
{
    for (int i = 0; i < 20; ++i) {
        HWND wnd = FindWindow(L"EFDeskPins", L"DeskPins");
        if (wnd) return wnd;
    }
    return 0;
}


void callback(int flag);


unsigned __stdcall dispatcher(void* args)
{
    const DispatcherParams& dp = *reinterpret_cast<const DispatcherParams*>(args);

    HWND wnd = findAppWindow();
    if (!wnd)
        throw L"error: could not find app window";

    if (dp.doneEvent)
        SetEvent(dp.doneEvent);
    return 0;
}*/


//------------------------------------------


int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int)
{
    // FIXME: sending cmdline options to already running instance; must use a different method
    /*
    if (argc__ > 1) {
        HANDLE doneEvent = app.prevInst.isRunning() ? CreateEvent(0, false, false, 0) : 0;
        const DispatcherParams dp(doneEvent);

        unsigned tid;
        HANDLE thread = (HANDLE)_beginthreadex(0, 0, dispatcher, (void*)args, 0, &tid);
        if (!thread)
            throw L"error: could not create thread";
        CloseHandle(thread);

        if (dp.doneEvent) {
            WaitForSingleObject(dp.doneEvent, INFINITE);
            CloseHandle(dp.doneEvent);
            dp.doneEvent = 0;
            return 0;
        }
    }*/

    app.inst = inst;

    // load settings as soon as possible
    opt.load();

    // setup UI dll & help early to get correct language msgs
    if (!app.loadResMod(opt.uiFile.c_str(), 0)) opt.uiFile = L"";
    app.help.init(app.inst, opt.helpFile);

    if (!app.chkPrevInst() || !app.initComctl())
        return 0;

    if (!app.regWndCls() || !app.createMainWnd(opt)) {
        Util::App::error(0, Util::Res::ResStr(IDS_ERR_WNDCLSREG));
        return 0;
    }

    MSG msg;
    while (GetMessage(&msg, 0, 0, 0)) {
        //if (app.activeModelessDlg && IsDialogMessage(app.activeModelessDlg, &msg))
        //    continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    opt.save();

    return msg.wParam;
}
