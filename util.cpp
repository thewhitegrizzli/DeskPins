#include "stdafx.h"
#include "pinwnd.h"
#include "util.h"
#include "resource.h"


bool Util::Wnd::isWndRectEmpty(HWND wnd)
{
    RECT rc;
    return GetWindowRect(wnd, &rc) && IsRectEmpty(&rc);
}


bool Util::Wnd::isChild(HWND wnd)
{
    return (ef::Win::WndH(wnd).getStyle() & WS_CHILD) != 0;
}


HWND Util::Wnd::getNonChildParent(HWND wnd)
{
    while (isChild(wnd))
        wnd = GetParent(wnd);

    return wnd;
}


HWND Util::Wnd::getTopParent(HWND wnd /*, bool mustBeVisible*/)
{
    // ------------------------------------------------------
    // NOTE: 'mustBeVisible' is not used currently
    // ------------------------------------------------------
    // By setting the 'mustBeVisible' flag,
    // more constraints are applied to the ultimate parent searching:
    //
    // #1: Stop when parent is invisible (e.g. the shell's Display Properties
    //     which has a hidden parent from RunDll)
    // #2: Stop when parent rect is null. This is the case with VCL apps
    //     where a parent window (TApplication class) is created as a proxy
    //     for app functionality (it has WS_VISIBLE, but the width/height are 0)
    //
    // The pinning engine handles invisible wnds (via the proxy mechanism).
    // 'mustBeVisible' is used mostly for user interaction
    // (e.g. peeking a window's attributes)
    // ------------------------------------------------------

    // go up the window chain to find the ultimate top-level wnd
    // for WS_CHILD wnds use GetParent()
    // for the rest use GetWindow(GW_OWNER)
    //
    for (;;) {
        HWND parent = isChild(wnd)
            ? GetParent(wnd)
            : GetWindow(wnd, GW_OWNER);

        if (!parent || parent == wnd) break;
        //if (mustBeVisible && !IsWindowVisible(parent) || IsWndRectEmpty(parent))
        //  break;
        wnd = parent;
    }

    return wnd;
}


bool Util::Wnd::isProgManWnd(HWND wnd)
{ 
    return Util::Text::strimatch(ef::Win::WndH(wnd).getClassName().c_str(), L"ProgMan")
        && Util::Text::strimatch(ef::Win::WndH(wnd).getText().c_str(), L"Program Manager");
}


bool Util::Wnd::isTaskBar(HWND wnd)
{
    return Util::Text::strimatch(ef::Win::WndH(wnd).getClassName().c_str(), L"Shell_TrayWnd");
}


bool Util::Wnd::isTopMost(HWND wnd)
{
    return (ef::Win::WndH(wnd).getExStyle() & WS_EX_TOPMOST) != 0;
}


bool Util::Wnd::isVCLAppWnd(HWND wnd)
{
    return Util::Text::strimatch(L"TApplication", ef::Win::WndH(wnd).getClassName().c_str()) 
        && Util::Wnd::isWndRectEmpty(wnd);
}


void Util::App::error(HWND wnd, LPCWSTR s)
{
    Util::Res::ResStr caption(IDS_ERRBOXTTITLE, 50, reinterpret_cast<DWORD>(::App::APPNAME));
    MessageBox(wnd, s, caption, MB_ICONSTOP | MB_TOPMOST);
}


void Util::App::warning(HWND wnd, LPCWSTR s)
{
    Util::Res::ResStr caption(IDS_WRNBOXTTITLE, 50, reinterpret_cast<DWORD>(::App::APPNAME));
    MessageBox(wnd, s, caption, MB_ICONWARNING | MB_TOPMOST);
}


std::wstring Util::App::getHelpFileDescr(const std::wstring& path, const std::wstring& name)
{
    // Data layout:
    //  {CHM file data...}
    //  WCHAR data[size]  // no NULs
    //  DWORD size
    //  DWORD sig

    const DWORD CHM_MARKER_SIG = 0xefda7a00;  // from chmmark.py
    std::wstring ret;
    DWORD sig, len;
    ef::Win::AutoFileH file = ef::Win::FileH::create(path + name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (file != INVALID_HANDLE_VALUE &&
        file.setPosFromEnd32(0) &&
        Util::Sys::readFileBack(file, &sig, sizeof(sig)) && 
        sig == CHM_MARKER_SIG &&
        Util::Sys::readFileBack(file, &len, sizeof(len)))
    {
        boost::scoped_array<char> buf(new char[len]);
        if (Util::Sys::readFileBack(file, buf.get(), len)) {
            boost::scoped_array<WCHAR> wbuf(new WCHAR[len]);
            if (MultiByteToWideChar(CP_THREAD_ACP, 0, buf.get(), len, wbuf.get(), len))
                ret.assign(wbuf.get(), len);
        }
    }
    return ret;
}


bool Util::Sys::getScrSize(SIZE& sz)
{
    return ((sz.cx = GetSystemMetrics(SM_CXSCREEN)) != 0 &&
        (sz.cy = GetSystemMetrics(SM_CYSCREEN)) != 0);
}


void Util::App::pinWindow(HWND wnd, HWND hitWnd, int trackRate, bool silent)
{
    int err = 0, wrn = 0;

    if (!hitWnd)
        wrn = IDS_ERR_COULDNOTFINDWND;
    else if (Util::Wnd::isProgManWnd(hitWnd))
        wrn = IDS_ERR_CANNOTPINDESKTOP;
    // NOTE: after creating the layer wnd, the taskbar becomes non-topmost;
    // use this check to avoid pinning it
    else if (Util::Wnd::isTaskBar(hitWnd))
        wrn = IDS_ERR_CANNOTPINTASKBAR;
    else if (Util::Wnd::isTopMost(hitWnd))
        wrn = IDS_ERR_ALREADYTOPMOST;
    // hidden wnds are handled by the proxy mechanism
    //else if (!IsWindowVisible(hitWnd))
    //  Error(wnd, "Cannot pin a hidden window");
    else {
        // create a pin wnd
        HWND pin = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            PinWnd::className,
            L"",
            WS_POPUP | WS_VISIBLE,
            0, 0, 0, 0,   // real pos/size set on wnd assignment
            0, 0, app.inst, 0);

        if (!pin)
            err = IDS_ERR_PINCREATE;
        else if (!SendMessage(pin, ::App::WM_PIN_ASSIGNWND, WPARAM(hitWnd), trackRate)) {
            err = IDS_ERR_PINWND;
            DestroyWindow(pin);
        }
    }

    if (!silent && (err || wrn)) {
        if (err)
            Util::App::error(wnd, Util::Res::ResStr(err));
        else
            Util::App::warning(wnd, Util::Res::ResStr(wrn));
    }

}


// If the specified window (top parent) is pinned, 
// return the pin wnd's handle; otherwise return 0.
//
HWND Util::App::hasPin(HWND wnd)
{
    // enumerate all pin windows
    HWND pin = 0;
    while ((pin = FindWindowEx(0, pin, PinWnd::className, 0)) != 0)
        //if (GetParent(pin) == wnd)
        if (HWND(SendMessage(pin, ::App::WM_PIN_GETPINNEDWND, 0, 0)) == wnd)
            return pin;

    return 0;
}


void Util::App::togglePin(HWND wnd, HWND target, int trackRate)
{
    target = Util::Wnd::getTopParent(target);
    HWND pin = hasPin(target);
    if (pin)
        DestroyWindow(pin);
    else
        Util::App::pinWindow(wnd, target, trackRate);
}


void Util::App::markWnd(HWND wnd, bool mode)
{
    const int blinkDelay = 50;  // msec
    // thickness of highlight border
    const int width = 3;
    // first val can vary; second should be zero
    const int flashes = mode ? 1 : 0;
    // amount to deflate if wnd is maximized, to make the highlight visible
    const int zoomFix = IsZoomed(wnd) ? GetSystemMetrics(SM_CXFRAME) : 0;

    // when composition is enabled, drawing on the glass frame is prohibited
    // (GetWindowDC() returns a DC that clips the frame); in that case,
    // we use the screen DC and draw a simple rect (since GetWindowRgn()
    // returns ERROR); this rect overlaps other windows in front of the target,
    // but it's better than nothing
    const bool composition = app.dwm.isCompositionEnabled();

    HDC dc = composition ? GetDC(0) : GetWindowDC(wnd);
    if (dc) {
        int orgRop2 = SetROP2(dc, R2_XORPEN);

        HRGN rgn = CreateRectRgn(0,0,0,0);
        bool hasRgn = GetWindowRgn(wnd, rgn) != ERROR;
        const int loops = flashes*2+1;

        if (hasRgn) {
            for (int m = 0; m < loops; ++m) {
                FrameRgn(dc, rgn, HBRUSH(GetStockObject(WHITE_BRUSH)), width, width);
                GdiFlush();
                if (mode && m < loops-1)
                    Sleep(blinkDelay);
            }
        }
        else {
            RECT rc;
            GetWindowRect(wnd, &rc);
            if (!composition)
                OffsetRect(&rc, -rc.left, -rc.top);
            InflateRect(&rc, -zoomFix, -zoomFix);

            HGDIOBJ orgPen = SelectObject(dc, GetStockObject(WHITE_PEN));
            HGDIOBJ orgBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));

            RECT tmp;
            for (int m = 0; m < loops; ++m) {
                CopyRect(&tmp, &rc);
                for (int n = 0; n < width; ++n) {
                    Rectangle(dc, tmp.left, tmp.top, tmp.right, tmp.bottom);
                    InflateRect(&tmp, -1, -1);
                }
                GdiFlush();
                if (mode && m < loops-1)
                    Sleep(blinkDelay);
            }
            SelectObject(dc, orgBrush);
            SelectObject(dc, orgPen);
        }

        SetROP2(dc, orgRop2);
        ReleaseDC(composition ? 0 : wnd, dc);
        DeleteObject(rgn);
    }

}


std::wstring Util::App::getLangFileDescr(const std::wstring& path, const std::wstring& file)
{
    HINSTANCE inst = file.empty() ? app.inst : LoadLibrary((path+file).c_str());
    WCHAR buf[100];
    if (!inst || !LoadString(inst, IDS_LANG, buf, sizeof(buf)))
        *buf = '\0';
    if (!file.empty() && inst)   // release inst if we had to load it
        FreeLibrary(inst);
    return buf;
}


HMENU Util::Res::LoadLocalizedMenu(LPCTSTR lpMenuName)
{
    if (app.resMod) {
        HMENU ret = LoadMenu(app.resMod, lpMenuName);
        if (ret)
            return ret;
    }
    return LoadMenu(app.inst, lpMenuName);
}


HMENU Util::Res::LoadLocalizedMenu(WORD id)
{
    return Util::Res::LoadLocalizedMenu(MAKEINTRESOURCE(id));
}


namespace {
    bool isLastErrResNotFound()
    {
        DWORD e = GetLastError();
        return e == ERROR_RESOURCE_DATA_NOT_FOUND ||
            e == ERROR_RESOURCE_TYPE_NOT_FOUND ||
            e == ERROR_RESOURCE_NAME_NOT_FOUND ||
            e == ERROR_RESOURCE_LANG_NOT_FOUND;
    }
}


int Util::Res::LocalizedDialogBoxParam(LPCTSTR lpTemplate, HWND hParent, DLGPROC lpDialogFunc, LPARAM dwInit)
{
    if (app.resMod) {
        int ret = DialogBoxParam(app.resMod, lpTemplate, hParent, lpDialogFunc, dwInit);
        if (ret != -1 || !isLastErrResNotFound())
            return ret;
    }
    return DialogBoxParam(app.inst, lpTemplate, hParent, lpDialogFunc, dwInit);
}


int Util::Res::LocalizedDialogBoxParam(WORD id, HWND hParent, DLGPROC lpDialogFunc, LPARAM dwInit)
{
    return Util::Res::LocalizedDialogBoxParam(MAKEINTRESOURCE(id), hParent, lpDialogFunc, dwInit);
}


HWND Util::Res::CreateLocalizedDialog(LPCTSTR lpTemplate, HWND hParent, DLGPROC lpDialogFunc)
{
    if (app.resMod) {
        HWND ret = CreateDialog(app.resMod, lpTemplate, hParent, lpDialogFunc);
        if (ret || !isLastErrResNotFound())
            return ret;
    }
    return CreateDialog(app.inst, lpTemplate, hParent, lpDialogFunc);
}


HWND Util::Res::CreateLocalizedDialog(WORD id, HWND hParent, DLGPROC lpDialogFunc)
{
    return Util::Res::CreateLocalizedDialog(MAKEINTRESOURCE(id), hParent, lpDialogFunc);
}


void Util::Res::ResStr::init(UINT id, size_t bufLen, DWORD_PTR* params) {
    str = new WCHAR[bufLen];
    if (!app.resMod || !LoadString(app.resMod, id, str, bufLen))
        LoadString(app.inst, id, str, bufLen);
    if (params) {
        DWORD flags = FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY;
        va_list* va = reinterpret_cast<va_list*>(params);
        if (!FormatMessage(flags, std::wstring(str).c_str(), 0, 0, str, bufLen, va))
            *str = 0;
    }
}


bool Util::Gfx::rectContains(const RECT& rc1, const RECT& rc2)
{
    return rc2.left   >= rc1.left
        && rc2.top    >= rc1.top
        && rc2.right  <= rc1.right
        && rc2.bottom <= rc1.bottom;
}


// enable/disable all ctrls that lie inside the specified ctrl 
// (usually a group, or maybe a tab, etc)
void Util::Wnd::enableGroup(HWND wnd, int id, bool mode)
{
    HWND container = GetDlgItem(wnd, id);
    RECT rc;
    GetWindowRect(container, &rc);

    // deflate a bit (4 DLUs) to be on the safe side (do we need this?)
    RECT rc2 = {0, 0, 4, 4};
    MapDialogRect(wnd, &rc2);
    InflateRect(&rc, rc2.left-rc2.right, rc2.top-rc2.bottom);

    for (HWND child = GetWindow(wnd, GW_CHILD); 
        child; child = GetWindow(child, GW_HWNDNEXT)) {
            if (child == container)
                continue;
            GetWindowRect(child, &rc2);
            if (Util::Gfx::rectContains(rc, rc2))
                EnableWindow(child, mode);
    }

}


std::vector<std::wstring> Util::Sys::getFiles(std::wstring mask)
{
    std::vector<std::wstring> ret;
    for (ef::Win::FileFinder fde(mask, ef::Win::FileFinder::files); fde; ++fde)
        ret.push_back(fde.getName());
    return ret;
}


bool Util::Sys::readFileBack(HANDLE file, void* buf, int bytes)
{
    DWORD read;
    return SetFilePointer(file, -bytes, 0, FILE_CURRENT) != -1
        && ReadFile(file, buf, bytes, &read, 0)
        && int(read) == bytes
        && SetFilePointer(file, -bytes, 0, FILE_CURRENT) != -1;
}


COLORREF Util::Clr::light(COLORREF clr)
{
    double r = GetRValue(clr) / 255.0;
    double g = GetGValue(clr) / 255.0;
    double b = GetBValue(clr) / 255.0;
    double h, l, s;
    ef::RGBtoHLS(r, g, b, h, l, s);

    l = min(1, l+0.25);

    ef::HLStoRGB(h, l, s, r, g, b);
    return RGB(r*255, g*255, b*255);
}


COLORREF Util::Clr::dark(COLORREF clr)
{
    double r = GetRValue(clr) / 255.0;
    double g = GetGValue(clr) / 255.0;
    double b = GetBValue(clr) / 255.0;
    double h, l, s;
    ef::RGBtoHLS(r, g, b, h, l, s);

    l = max(0, l-0.25);

    ef::HLStoRGB(h, l, s, r, g, b);
    return RGB(r*255, g*255, b*255);
}


BOOL Util::Wnd::moveWindow(HWND wnd, const RECT& rc, BOOL repaint)
{
    return MoveWindow(wnd, rc.left, rc.top, 
        rc.right-rc.left, rc.bottom-rc.top, repaint);
}


BOOL Util::Gfx::rectangle(HDC dc, const RECT& rc)
{
    return Rectangle(dc, rc.left, rc.top, rc.right, rc.bottom);
}


bool Util::Wnd::psChanged(HWND page)
{
    return !!PropSheet_Changed(GetParent(page), page);
}


// removes the first accelerator prefix ('&')
// from a string and returns the result
std::wstring Util::Text::remAccel(std::wstring s)
{
    std::wstring::size_type i = s.find_first_of(L"&");
    if (i != std::wstring::npos) s.erase(i, 1);
    return s;
}


bool Util::Gfx::getBmpSize(HBITMAP bmp, SIZE& sz)
{
    BITMAP bm;
    if (!GetObject(bmp, sizeof(bm), &bm))
        return false;
    sz.cx = bm.bmWidth;
    sz.cy = abs(bm.bmHeight);
    return true;
}


bool Util::Gfx::getBmpSizeAndBpp(HBITMAP bmp, SIZE& sz, int& bpp)
{
    BITMAP bm;
    if (!GetObject(bmp, sizeof(bm), &bm))
        return false;
    sz.cx = bm.bmWidth;
    sz.cy = abs(bm.bmHeight);
    bpp   = bm.bmBitsPixel;
    return true;
}


// Convert white/lgray/dgray of bmp to shades of 'clr'.
// NOTE: Bmp must *not* be selected in any DC for this to succeed.
//
bool Util::Gfx::remapBmpColors(HBITMAP bmp, COLORREF clrs[][2], int cnt)
{
    bool ok = false;
    SIZE sz;
    int bpp;
    if (getBmpSizeAndBpp(bmp, sz, bpp)) {
        if (HDC dc = CreateCompatibleDC(0)) {
            if (HBITMAP orgBmp = HBITMAP(SelectObject(dc, bmp))) {
                if (bpp == 16) {
                    // In 16-bpp modes colors get changed,
                    // e.g. light gray (0xC0C0C0) becomes 0xC6C6C6
                    // GetNearestColor() only works for paletized modes,
                    // so we'll have to patch our source colors manually
                    // using Nearest16BppColor.
                    ef::Win::Nearest16BppColor nearClr;
                    for (int n = 0; n < cnt; ++n)
                        clrs[n][0] = nearClr(clrs[n][0]);
                }
                for (int y = 0; y < sz.cy; ++y) {
                    for (int x = 0; x < sz.cx; ++x) {
                        COLORREF clr = GetPixel(dc, x, y);
                        for (int n = 0; n < cnt; ++n)
                            if (clr == clrs[n][0])
                                SetPixelV(dc, x, y, clrs[n][1]);
                    }
                }
                ok = true;
                SelectObject(dc, orgBmp);
            }
            DeleteDC(dc);
        }
    }

    //ostringstream oss;
    //oss << hex << setfill('0');
    //for (set<COLORREF>::const_iterator it = clrSet.begin(); it != clrSet.end(); ++it)
    //  oss << setw(6) << *it << "\r\n";
    //MessageBoxA(app.mainWnd, oss.str().c_str(), "Unique clrs", 0);

    return ok;
}


// Returns the part of a string after the last occurrence of a token.
// Example: substrAfter("foobar", "oo") --> "bar"
// Returns "" on error.
std::wstring Util::Text::substrAfterLast(const std::wstring& s, const std::wstring& delim)
{
    std::wstring::size_type i = s.find_last_of(delim);
    return i == std::wstring::npos ? L"" : s.substr(i + delim.length());
}


bool Util::Sys::isWin8orGreater()
{
    return ef::Win::OsVer().majMin() >= ef::Win::packVer(6, 2);
}
