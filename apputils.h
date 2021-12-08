#pragma once


// ABC for monitoring system window creation.
// Post a message to the client window whenever a window is created.
//
class WindowCreationMonitor {
public:
    virtual ~WindowCreationMonitor() {}
    virtual bool init(HWND wnd, int msgId) = 0;
    virtual bool term() = 0;
};


// Window creation monitor that uses SetWinEventHook().
//
class EventHookWindowCreationMonitor : public WindowCreationMonitor, boost::noncopyable {
public:
    EventHookWindowCreationMonitor() {}
    ~EventHookWindowCreationMonitor() { term(); }

    bool init(HWND wnd, int msgId);
    bool term();

private:
    static HWINEVENTHOOK hook;
    static HWND wnd;
    static int msgId;

    static VOID CALLBACK proc(HWINEVENTHOOK hook, DWORD event,
        HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
};

    
// Window creation monitor that uses a global hook in a DLL (deprecated).
//
class HookDllWindowCreationMonitor : public WindowCreationMonitor, boost::noncopyable {
private:
    typedef bool (*initF)(HWND wnd, int msgId);
    typedef bool (*termF)();

public:
    HookDllWindowCreationMonitor() : dll(0), dllInitFunc(0), dllTermFunc(0) {}
    ~HookDllWindowCreationMonitor() { term(); }

    bool init(HWND wnd, int msgId);
    bool term();

private:
    HMODULE dll;
    initF dllInitFunc;
    termF dllTermFunc;
};


class Options;


// Autopin checking of created windows.
// Remembers the time each window is added, so when checking
// it only processes those that have passed the autopin delay.
//
class PendingWindows {
public:
    void add(HWND wnd);
    void check(HWND wnd, const Options& opt);

protected:
    struct Entry {
        HWND  wnd;
        DWORD time;
        Entry(HWND h = 0, DWORD t = 0) : wnd(h), time(t) {}
    };
    std::vector<Entry> entries;

    bool timeToChkWnd(DWORD t, const Options& opt);
    bool checkWnd(HWND target, const Options& opt);

};
