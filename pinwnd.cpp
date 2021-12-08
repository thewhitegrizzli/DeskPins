#include "stdafx.h"
#include "util.h"
#include "pinshape.h"
#include "pinwnd.h"
#include "resource.h"


LPCWSTR PinWnd::className = L"EFPinWnd";


ATOM PinWnd::registerClass()
{
    WNDCLASS wc = {};
    wc.lpfnWndProc   = proc;
    wc.cbWndExtra    = sizeof(void*);  // data object ptr
    wc.hInstance     = app.inst;
    wc.hCursor       = LoadCursor(app.inst, MAKEINTRESOURCE(IDC_REMOVEPIN));
    wc.lpszClassName = className;
    return RegisterClass(&wc);
}


LRESULT CALLBACK PinWnd::proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_NCCREATE) {
        Data::create(wnd, app.mainWnd);
        return true;
    }
    else if (msg == WM_NCDESTROY) {
        Data::destroy(wnd);
        return 0;
    }
    Data* pd = Data::get(wnd);
    if (pd) {
        switch (msg) {
            case WM_CREATE:         return evCreate(wnd, *pd);
            case WM_DESTROY:        return evDestroy(wnd, *pd), 0;
            case WM_TIMER:          return evTimer(wnd, *pd, wparam), 0;
            case WM_PAINT:          return evPaint(wnd, *pd), 0;
            case WM_LBUTTONDOWN:    return evLClick(wnd, *pd), 0;
            case App::WM_PIN_RESETTIMER:   return evPinResetTimer(wnd, *pd, int(wparam)), 0;
            case App::WM_PIN_ASSIGNWND:    return evPinAssignWnd(wnd, *pd, HWND(wparam), int(lparam));
            case App::WM_PIN_GETPINNEDWND: return LRESULT(evGetPinnedWnd(wnd, *pd));
        }
    }
    return DefWindowProc(wnd, msg, wparam, lparam);
}


LRESULT PinWnd::evCreate(HWND wnd, Data& pd)
{
    // send 'pin created' notification
    PostMessage(pd.callbackWnd, App::WM_PINSTATUS, WPARAM(wnd), true);

    // initially place pin in the middle of the screen;
    // later it will be positioned on the pinned wnd's caption
    RECT rc;
    GetWindowRect(wnd, &rc);
    int wndW = rc.right - rc.left;
    int wndH = rc.bottom - rc.top;
    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(wnd, 0, (scrW-wndW)/2, (scrH-wndH)/2, 0, 0,
        SWP_NOZORDER | SWP_NOSIZE);

    return 0;
}


void PinWnd::evDestroy(HWND wnd, Data& pd)
{
    if (pd.topMostWnd) {
        SetWindowPos(pd.topMostWnd, HWND_NOTOPMOST, 0,0,0,0, 
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); // NOTE: added noactivate
        pd.topMostWnd = pd.proxyWnd = 0;
        pd.proxyMode = false;
    }

    // send 'pin destroyed' notification
    PostMessage(pd.callbackWnd, App::WM_PINSTATUS, WPARAM(wnd), false);
}


// Helper to collect all non-child windows of a thread.
//
class ThreadWnds {
public:
    ThreadWnds(HWND wnd) : appWnd(wnd) {}
    int count() const { return wndList.size(); }
    HWND operator[](int n) const { return wndList[n]; }

    bool collect()
    {
        DWORD threadId = GetWindowThreadProcessId(appWnd, 0);
        return !!EnumThreadWindows(threadId, (WNDENUMPROC)enumProc, LPARAM(this));
    }

protected:
    HWND appWnd;
    std::vector<HWND> wndList;

    static BOOL CALLBACK enumProc(HWND wnd, LPARAM param)
    {
        ThreadWnds& obj = *reinterpret_cast<ThreadWnds*>(param);
        if (wnd == obj.appWnd || GetWindow(wnd, GW_OWNER) == obj.appWnd)
            obj.wndList.push_back(wnd);
        return true;  // continue enumeration
    }

};


// - get the visible, top-level windows
// - if any enabled wnds are after any disabled
//   get the last enabled wnd; otherwise exit
// - move all disabled wnds (starting from the bottom one)
//   behind the last enabled
//
void PinWnd::fixPopupZOrder(HWND appWnd)
{
    // - find the most visible, disabled, top-level wnd (wnd X)
    // - find all non-disabled, top-level wnds and place them
    //   above wnd X

    // get all non-child wnds
    ThreadWnds threadWnds(appWnd);
    if (!threadWnds.collect())
        return;

    // HACK: here I'm assuming that EnumThreadWindows returns
    // HWNDs acoording to their z-order (is this documented anywhere?)

    // Reordering is needed if any disabled wnd
    // is above any enabled one.
    bool needReordering = false;
    for (int n = 1; n < threadWnds.count(); ++n) {
        if (!IsWindowEnabled(threadWnds[n-1]) && IsWindowEnabled(threadWnds[n])) {
            needReordering = true;
            break;
        }
    }

    if (!needReordering)
        return;

    // find last enabled
    HWND lastEnabled = 0;
    for (int n = threadWnds.count()-1; n >= 0; --n) {
        if (IsWindowEnabled(threadWnds[n])) {
            lastEnabled = threadWnds[n];
            break;
        }
    }

    // none enabled? bail out
    if (!lastEnabled)
        return;

    // move all disabled (starting from the last) behind the last enabled
    for (int n = threadWnds.count()-1; n >= 0; --n) {
        if (!IsWindowEnabled(threadWnds[n])) {
            SetWindowPos(threadWnds[n], lastEnabled, 0,0,0,0, 
                SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER);
        }
    }

}


void PinWnd::evTimer(HWND wnd, Data& pd, UINT id)
{
    if (id != 1) return;

    // Does the app wnd still exist?
    // We must check this because, when the pinned wnd closes,
    // the pin is just hidden, not destroyed.
    if (!IsWindow(pd.topMostWnd)) {
        pd.topMostWnd = pd.proxyWnd = 0;
        pd.proxyMode = false;
        DestroyWindow(wnd);
        return;
    }

    if (pd.proxyMode
        && (!pd.proxyWnd || !IsWindowVisible(pd.proxyWnd))
        && !selectProxy(wnd, pd))
        return;

    // bugfix: disabled proxy mode check so that FixVisible()
    // is called even on normal apps that hide the window on minimize
    // (just like our About dlg)
    if (/*pd->proxyMode &&*/ !fixVisible(wnd, pd))
        return;

    if (pd.proxyMode && !IsWindowEnabled(pd.topMostWnd))
        fixPopupZOrder(pd.topMostWnd);

    fixTopStyle(wnd, pd);
    placeOnCaption(wnd, pd);
}


void PinWnd::evPaint(HWND wnd, Data& pd)
{
    PAINTSTRUCT ps;
    if (HDC dc = BeginPaint(wnd, &ps)) {
        if (app.pinShape.getBmp()) {
            if (HDC memDC = CreateCompatibleDC(0)) {
                if (HBITMAP orgBmp = HBITMAP(SelectObject(memDC, app.pinShape.getBmp()))) {
                    BitBlt(dc, 0,0,app.pinShape.getW(),app.pinShape.getH(), memDC, 0,0, SRCCOPY);
                    SelectObject(memDC, orgBmp);
                }
                DeleteDC(memDC);
            }
        }
        EndPaint(wnd, &ps);
    }
}


void PinWnd::evLClick(HWND wnd, Data& pd)
{
    DestroyWindow(wnd);
}


bool PinWnd::evPinAssignWnd(HWND wnd, Data& pd, HWND target, int pollRate)
{
    // this shouldn't happen; it means the pin is already used
    if (pd.topMostWnd) return false;

    pd.topMostWnd = target;

    // decide proxy mode
    if (!IsWindowVisible(target) || Util::Wnd::isWndRectEmpty(target) || Util::Wnd::isVCLAppWnd(target)) {
        // set proxy mode flag; we'll find a proxy wnd later
        pd.proxyMode = true;
    }

    if (pd.getPinOwner()) {
        // docs say not to set GWL_HWNDPARENT, but it works nevertheless
        // (SetParent() only works with windows of the same process)
        SetLastError(0);
        if (!SetWindowLong(wnd, GWL_HWNDPARENT, LONG(pd.getPinOwner())) && GetLastError())
            Util::App::error(wnd, Util::Res::ResStr(IDS_ERR_SETPINPARENTFAIL));
    }

    if (!SetWindowPos(pd.topMostWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE))
        Util::App::error(wnd, Util::Res::ResStr(IDS_ERR_SETTOPMOSTFAIL));

    // set wnd region
    if (app.pinShape.getRgn()) {
        HRGN rgnCopy = CreateRectRgn(0,0,0,0);
        if (CombineRgn(rgnCopy, app.pinShape.getRgn(), 0, RGN_COPY) != ERROR) {
            if (!SetWindowRgn(wnd, rgnCopy, false))
                DeleteObject(rgnCopy);
        }
    }

    // set pin size, move it on the caption, and make it visible
    SetWindowPos(wnd, 0, 0,0,app.pinShape.getW(),app.pinShape.getH(), 
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE); // NOTE: added SWP_NOACTIVATE
    placeOnCaption(wnd, pd);
    ShowWindow(wnd, SW_SHOW);

    SetForegroundWindow(pd.topMostWnd);

    // start polling timer
    SetTimer(wnd, 1, pollRate, 0);

    return true;
}


HWND PinWnd::evGetPinnedWnd(HWND wnd, Data& pd)
{
    return pd.topMostWnd;
}


void PinWnd::evPinResetTimer(HWND wnd, Data& pd, int pollRate)
{
    // only set it if there's a pinned window
    if (pd.topMostWnd)
        SetTimer(wnd, 1, pollRate, 0);
}


// Patch for VCL apps (owner-of-owner problem).
// If the pinned and pin wnd visiblity states are different
// change pin state to pinned wnd state.
// Return whether the pin is visible
// (if so, the caller should perform further adjustments)
bool PinWnd::fixVisible(HWND wnd, const Data& pd)
{
    // insanity check
    if (!IsWindow(pd.topMostWnd)) return false;

    HWND pinOwner = pd.getPinOwner();
    if (!pinOwner) return false;

    // IsIconic() is crucial; without it we cannot restore the minimized
    // wnd by clicking on the taskbar button
    bool ownerVisible = IsWindowVisible(pinOwner) && !IsIconic(pinOwner);
    bool pinVisible = !!IsWindowVisible(wnd);
    if (ownerVisible != pinVisible)
        ShowWindow(wnd, ownerVisible ? SW_SHOWNOACTIVATE : SW_HIDE);

    // return whether the pin is visible now
    return ownerVisible;
}


// patch for VCL apps (clearing of WS_EX_TOPMOST bit)
void PinWnd::fixTopStyle(HWND wnd, const Data& pd)
{
    if (!(ef::Win::WndH(pd.topMostWnd).getExStyle() & WS_EX_TOPMOST))
        SetWindowPos(pd.topMostWnd, HWND_TOPMOST, 0,0,0,0, 
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}


void PinWnd::placeOnCaption(HWND wnd, const Data& pd)
{
    HWND pinOwner = pd.getPinOwner();
    if (!pinOwner) return;

    // Move the pin on owner's caption
    //

    // Calc the number of caption buttons
    int btnCnt = 0;
    LONG style   = ef::Win::WndH(pinOwner).getStyle();
    LONG exStyle = ef::Win::WndH(pinOwner).getExStyle();
    LONG minMaxMask = WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    bool hasMinOrMax  = (style & minMaxMask) != 0;
    bool hasMinAndMax = (style & minMaxMask) == minMaxMask;
    bool hasClose = (style & WS_SYSMENU) != 0;
    bool hasHelp  = (exStyle & WS_EX_CONTEXTHELP) != 0;
    if (style & WS_SYSMENU) {  // other buttons appear only if this is set
        ++btnCnt;
        if (hasMinOrMax)
            btnCnt += 2;
        // Win XP/2000 erroneously allow a non-functional help button 
        // when either (not both!) of the min/max buttons are enabled
        if (hasHelp && (!hasMinOrMax || (ef::Win::OsVer().major() == 5 && !hasMinAndMax)))
            ++btnCnt;
    }

    RECT pin, pinned;
    GetWindowRect(wnd, &pin);
    GetWindowRect(pinOwner, &pinned);
    int x = pinned.right - (btnCnt*GetSystemMetrics(SM_CXSIZE) + app.pinShape.getW() + 10); // add a small padding
    SetWindowPos(wnd, 0, x, pinned.top+3, 0, 0, 
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

    // reveal VCL app wnd :)
    //if (rcPinned.top >= rcPinned.bottom || rcPinned.left >= rcPinned.right)
    //  MoveWindow(pd.pinnedWnd, rcPinned.left, rcPinned.top, 200, 100, true);

}


bool PinWnd::selectProxy(HWND wnd, const Data& pd)
{
    HWND appWnd = pd.topMostWnd;
    if (!IsWindow(appWnd)) return false;

    DWORD thread = GetWindowThreadProcessId(appWnd, 0);
    return EnumThreadWindows(thread, (WNDENUMPROC)enumThreadWndProc, 
        LPARAM(wnd)) && pd.getPinOwner();
}


BOOL CALLBACK PinWnd::enumThreadWndProc(HWND wnd, LPARAM param)
{
    HWND pin = HWND(param);
    Data* pd = Data::get(pin);
    if (!pd)
        return false;

    if (GetWindow(wnd, GW_OWNER) == pd->topMostWnd) {
        if (IsWindowVisible(wnd) && !IsIconic(wnd) && !Util::Wnd::isWndRectEmpty(wnd)) {
            pd->proxyWnd = wnd;
            SetWindowLong(pin, GWL_HWNDPARENT, LONG(wnd));
            // we must also move it in front of the new owner
            // otherwise the wnd mgr will not do it until we select the new owner
            SetWindowPos(wnd, pin, 0,0,0,0, 
                SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
            return false;   // stop enumeration
        }
    }

    return true;  // continue enumeration
}
