#pragma once


// Program notification icon management.
//
class TrayIcon : boost::noncopyable {
public:
    TrayIcon(UINT msg, UINT id);
    ~TrayIcon();

    bool setWnd(HWND wnd_);

    bool create(HICON icon, LPCTSTR tip);
    bool destroy();

    bool setTip(LPCTSTR tip);
    bool setIcon(HICON icon);

private:
    HWND wnd;
    UINT id;
    UINT msg;
};
