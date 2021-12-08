// ico_clr.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"

static HINSTANCE hInst;

/*

  must try setting the generated icon to the tray, too...

  and try all BPP modes

*/


bool getBmpSize(HBITMAP hBmp, SIZE& sz)
{
  BITMAP bm;
  if (!GetObject(hBmp, sizeof(bm), &bm))
    return false;
  sz.cx = bm.bmWidth;
  sz.cy = bm.bmHeight >= 0 ? bm.bmHeight : -bm.bmHeight;
  return true;
}


bool colorizeBmp(HBITMAP hBmp)
{
  bool ok = false;
  SIZE sz;
  if (getBmpSize(hBmp, sz)) {
    if (HDC hDC = CreateCompatibleDC(0)) {
      if (HBITMAP hOrgBmp = HBITMAP(SelectObject(hDC, hBmp))) {

        /*
        COLORREF src[3] = { CLR_WHITE,  CLR_LGRAY, CLR_DGRAY };
        COLORREF dst[3] = { Light(clr), clr,       Dark(clr) };
        for (int y = 0; y < sz.cy; ++y) {
          for (int x = 0; x < sz.cx; ++x) {
            COLORREF clr = GetPixel(hDC, x, y);
                 if (clr == src[0])  SetPixelV(hDC, x, y, dst[0]);
            else if (clr == src[1])  SetPixelV(hDC, x, y, dst[1]);
            else if (clr == src[2])  SetPixelV(hDC, x, y, dst[2]);
          }
        }
        */
        for (int y = 0; y < sz.cy; ++y) {
          for (int x = 0; x < sz.cx; ++x) {
            int r = x * 255 / sz.cx;
            int g = y * 255 / sz.cy;
            int b = 0;
            SetPixelV(hDC, x, y, RGB(r,g,b));
          }
        }

        ok = true;
        SelectObject(hDC, hOrgBmp);
      }
      DeleteDC(hDC);
    }
  }
  return ok;
}


void test(HICON& hIcon)
{
  ICONINFO ii;
  if (GetIconInfo(hIcon, &ii)) {
    // (assuming non-monochrome)
    colorizeBmp(ii.hbmColor);
    HICON hNewIcon = CreateIconIndirect(&ii);
    if (hNewIcon) {
      DestroyIcon(hIcon);
      hIcon = hNewIcon;
    }
    // (assuming non-monochrome)
    // destroy bmps returned from GetIconInfo()
    DeleteObject(ii.hbmColor);
    DeleteObject(ii.hbmMask);
  }
}


class TrayIcon {
public:
  TrayIcon(HWND w)
  {
    nid.cbSize           = sizeof(NOTIFYICONDATA);
    nid.hWnd             = w;
    nid.uID              = 0;
    nid.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_APP;
    lstrcpy(nid.szTip, "ico_clr");

    displayed = false;
  }

 ~TrayIcon()
  {
    remove();
  }

  void add(HICON hIcon)
  {
    if (displayed) remove();
    nid.hIcon = hIcon;
    Shell_NotifyIcon(NIM_ADD, &nid);
    displayed = true;
  }

  void remove()
  {
    if (displayed) {
      Shell_NotifyIcon(NIM_DELETE, &nid);
      displayed = false;
    }
  }

protected:
  NOTIFYICONDATA nid;
  bool displayed;
};


#pragma warning (disable:4800)    // BOOL -> int
#pragma warning (disable:4786)    // browser info trunc to 255 chars
#include <fstream>
#include <iomanip>
#include <set>
#include <string>
using namespace std;



class Nearest16BppColor {
public:
  Nearest16BppColor() : hDC(0), hDib(0), hOrgBmp(0)
  {
    if ((hDC = CreateCompatibleDC(0)) != 0) {
      void* bits;
      BITMAPINFO bi;
      bi.bmiHeader.biSize          = sizeof(bi.bmiHeader);
      bi.bmiHeader.biWidth         = 1;
      bi.bmiHeader.biHeight        = 1;
      bi.bmiHeader.biPlanes        = 1;
      bi.bmiHeader.biBitCount      = 16;
      bi.bmiHeader.biCompression   = BI_RGB;
      bi.bmiHeader.biSizeImage     = 0;
      bi.bmiHeader.biXPelsPerMeter = 0;
      bi.bmiHeader.biYPelsPerMeter = 0;
      bi.bmiHeader.biClrUsed       = 0;
      bi.bmiHeader.biClrImportant  = 0;
      if ((hDib = CreateDIBSection(hDC, &bi,  DIB_RGB_COLORS, 
                                   &bits, 0, 0)) != 0)
      {
        hOrgBmp = HBITMAP(SelectObject(hDC, hDib));
      }
    }
  }
 ~Nearest16BppColor()
  {
    if (hOrgBmp) SelectObject(hDC, hOrgBmp);
    if (hDib) DeleteObject(hDib);
    if (hDC) DeleteDC(hDC);
  }
  bool valid() const
  {
    // hOrgBmp != 0 means that all steps in ctor succeeded
    return hOrgBmp;
  }
  COLORREF get(COLORREF clr) const
  {
    return valid() ? SetPixel(hDC, 0, 0, clr) : CLR_INVALID;
  }
  COLORREF operator()(COLORREF clr) const
  {
    return get(clr);
  }
protected:
  HDC     hDC;
  HBITMAP hDib, hOrgBmp;
};


/*
COLORREF GetNearest16BppColor(COLORREF clr)
{
  COLORREF ret = CLR_INVALID;
  if (HDC hDC = CreateCompatibleDC(0)) {
    void* bits;
    BITMAPINFO bi;
    bi.bmiHeader.biSize          = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth         = 1;
    bi.bmiHeader.biHeight        = 1;
    bi.bmiHeader.biPlanes        = 1;
    bi.bmiHeader.biBitCount      = 16;
    bi.bmiHeader.biCompression   = BI_RGB;
    bi.bmiHeader.biSizeImage     = 0;
    bi.bmiHeader.biXPelsPerMeter = 0;
    bi.bmiHeader.biYPelsPerMeter = 0;
    bi.bmiHeader.biClrUsed       = 0;
    bi.bmiHeader.biClrImportant  = 0;
    if (HBITMAP hDib = CreateDIBSection(hDC, &bi,  DIB_RGB_COLORS, &bits, 0, 0)) {
      if (HGDIOBJ orgBmp = SelectObject(hDC, hDib)) {
        ret = SetPixel(hDC, 0, 0, clr);
        SelectObject(hDC, orgBmp);
      }
      DeleteObject(hDib);
    }
    DeleteDC(hDC);
  }
  return ret;
}


COLORREF MyGetNearestColor(HDC hDC, COLORREF src)
{
  return SetPixel(hDC, 0, 0, src);
}
*/


string binBits(DWORD x)
{
  string ret;
  char bits[2] = {'0', '1'};
  for (int n = 0; n < 8; ++n, x >>= 1)
    ret = bits[x & 1] + ret;
  return ret;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static HICON hIcon;
  static TrayIcon* trayIcon;

  switch (uMsg) {
    case WM_CREATE: {
      hIcon = HICON(LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
      test(hIcon);
      trayIcon = new TrayIcon(hWnd);
      trayIcon->add(hIcon);

      /*
      set<COLORREF> clrSet;
      HDC hScrDC = GetDC(0);
      for (int n = 0; n < 256; ++n) {
        COLORREF clr = MyGetNearestColor(hScrDC, RGB(0,0,n));
        if (clrSet.find(clr) == clrSet.end()) clrSet.insert(clr);
      }
      ReleaseDC(0, hScrDC);

      const char* fname = "C:\\Documents and Settings\\Elias\\Desktop\\clrSet.txt";
      ofstream ofs(fname);
      ofs << hex << setfill('0');
      for (set<COLORREF>::const_iterator it = clrSet.begin(); it != clrSet.end(); ++it)
        ofs << setw(6) << *it << '\t' << binBits(*it) << "\r\n";
      */

      Nearest16BppColor nearClr;
      
      set<COLORREF> clrSet;
      char s[20];
      HDC hScrDC = GetDC(0);
      for (int b = 0; b < 255; ++b) {
        for (int g = 0; g < 255; ++g) {
          for (int r = 0; r < 255; ++r) {
            //COLORREF clr = MyGetNearestColor(hScrDC, RGB(r,g,b));
            //COLORREF clr = GetNearest16BppColor(RGB(r,g,b));
            COLORREF clr = nearClr(RGB(r,g,b));
            if (clrSet.find(clr) == clrSet.end()) clrSet.insert(clr);
          }
        }
        itoa(b, s, 10);
        TextOut(hScrDC, 0, 0, s, lstrlen(s));
      }
      ReleaseDC(0, hScrDC);
      MessageBox(hWnd, itoa(clrSet.size(),s,10), "Unique clrs", 0);

      break;
    }
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hDC = BeginPaint(hWnd, &ps);

      DrawIconEx(hDC, 0, 0, hIcon, 0, 0, 0, 0, DI_NORMAL);

      /*
      HDC hScrDC = GetDC(0);
      COLORREF clr = GetNearestColor(hScrDC, 0xc0c0c0));
      ReleaseDC(0, hScrDC);
      char s[40];
      TextOut(hDC, 0, 20, s, wsprintf(s, "%08x", clr));
      */

      EndPaint(hWnd, &ps);
      break;
    }
    case WM_DESTROY: {
      delete trayIcon;
      trayIcon = 0;

      if (hIcon) {
        DeleteObject(hIcon);
        hIcon = 0;
      }

      PostQuitMessage(0);
      break;
    }
    default: {
      return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
  }

  return 0;
}


bool RegWndCls()
{
  WNDCLASS wc;
  wc.style         = 0;
  wc.lpfnWndProc   = WndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = hInst;
  wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
  wc.hCursor       = LoadCursor(0, IDC_ARROW);
  wc.hbrBackground = HBRUSH(GetStockObject(WHITE_BRUSH));
  wc.lpszMenuName  = 0;
  wc.lpszClassName = "MainWnd";
  if (!RegisterClass(&wc)) return false;

  return true;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR szCmdLine, int cmdShow)
{
  hInst = hInstance;

  if (!RegWndCls())
    return 0;

  int w = 100;
  int h = 100;
  HWND hWnd = CreateWindow("MainWnd", "test",
    WS_OVERLAPPEDWINDOW,
    (GetSystemMetrics(SM_CXSCREEN)-w)/2, (GetSystemMetrics(SM_CYSCREEN)-h)/2,
    w, h,
    0, 0, hInstance, 0);

  ShowWindow(hWnd, cmdShow);
  UpdateWindow(hWnd);

  MSG msg;
  while (GetMessage(&msg, 0, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.wParam;
}
/*

1    _____    _____   .
2   / ___/\  / ___/\  .
3  / __/\\/ / __/\\/  .
4 /____/\  /_/\_\/    .
5 \____\/  \_\/       .

1    _____  _____   .
2   / ___/ / ___/   .
3  / __/  / __/     .
4 /____/ /_/        .

1  __    _
2 /__\ _|_
3 \__   |
4      /

1  __   __
2 |__  |__
3 |__  |

*/

//SPIF_UPDATEINIFILE 
//SPI_SETFONTSMOOTHINGCONTRAST