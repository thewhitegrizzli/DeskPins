#pragma once


// Small popup window in the shape of pin.
// This is fully responsible for making the target window topmost.
//
class PinWnd {
public:
    static ATOM registerClass();
    static LRESULT CALLBACK proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);
    static LPCWSTR className;

protected:
    // Window data object.
    //
    class Data {
    public:
        // Attach to pin wnd and return object or 0 on error.
        static Data* create(HWND pin, HWND callback) {
            Data* p = new Data(callback);
            SetLastError(0);
            if (!SetWindowLong(pin, 0, LONG(p)) && GetLastError()) {
                delete p;
                p = 0;
            }
            return p;
        }
        // Get object or 0 on error.
        static Data* get(HWND pin) {
            return reinterpret_cast<Data*>(GetWindowLong(pin, 0));
        }
        // Detach from pin wnd and delete.
        static void destroy(HWND pin) {
            Data* p = get(pin);
            if (!p)
                return;
            SetLastError(0);
            if (!SetWindowLong(pin, 0, 0) && GetLastError())
                return;
            delete p;
        }

        HWND callbackWnd;

        bool proxyMode;
        HWND topMostWnd;
        HWND proxyWnd;

        HWND getPinOwner() const {
            return proxyMode ? proxyWnd : topMostWnd;
        }

    private:
        Data(HWND wnd) : callbackWnd(wnd), proxyMode(false), topMostWnd(0), proxyWnd(0) {}
    };

    static BOOL CALLBACK enumThreadWndProc(HWND wnd, LPARAM param);
    static bool selectProxy(HWND wnd, const Data& pd);
    static void fixTopStyle(HWND wnd, const Data& pd);
    static void placeOnCaption(HWND wnd, const Data& pd);
    static bool fixVisible(HWND wnd, const Data& pd);
    static void fixPopupZOrder(HWND appWnd);

    static LRESULT evCreate(HWND wnd, Data& pd);
    static void evDestroy(HWND wnd, Data& pd);
    static void evTimer(HWND wnd, Data& pd, UINT id);
    static void evPaint(HWND wnd, Data& pd);
    static void evLClick(HWND wnd, Data& pd);
    static bool evPinAssignWnd(HWND wnd, Data& pd, HWND target, int pollRate);
    static HWND evGetPinnedWnd(HWND wnd, Data& pd);
    static void evPinResetTimer(HWND wnd, Data& pd, int pollRate);
};
