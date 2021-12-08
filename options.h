#pragma once

#include "util.h"
#include "resource.h"


// Hotkey item.
// Manages its own activation, persistence and UI interaction.
//
struct HotKey {
    int  id;
    UINT vk;
    UINT mod;

    explicit HotKey(int id_, UINT vk_ = 0, UINT mod_ = 0)
        : id(id_), vk(vk_), mod(mod_) {}

    bool operator==(const HotKey& other) const {
        return vk == other.vk && mod == other.mod;
    }
    bool operator!=(const HotKey& other) const {
        return !(*this == other);
    }

    bool set(HWND wnd) const
    {
        return !!RegisterHotKey(wnd, id, mod, vk);
    }

    bool unset(HWND wnd) const
    {
        return !!UnregisterHotKey(wnd, id);
    }

    bool load(ef::Win::RegKeyH& key, LPCWSTR val);
    bool save(ef::Win::RegKeyH& key, LPCWSTR val) const;

    void getUI(HWND wnd, int id)
    {
        LRESULT res = SendDlgItemMessage(wnd, id, HKM_GETHOTKEY, 0, 0);
        vk  = LOBYTE(res);
        mod = HIBYTE(res);
        if (!vk)
            mod = 0;
    }

    void setUI(HWND wnd, int id) const
    {
        SendDlgItemMessage(wnd, id, HKM_SETHOTKEY, MAKEWORD(vk, mod), 0);
    }

};


// Autopin rule.
// Manages its own persistence.
//
struct AutoPinRule {
    std::wstring descr;
    std::wstring ttl;
    std::wstring cls;
    bool enabled;

    AutoPinRule(const std::wstring& d = std::wstring(Util::Res::ResStr(IDS_NEWRULEDESCR)), 
        const std::wstring& t = L"", 
        const std::wstring& c = L"", 
        bool b = true) : descr(d), ttl(t), cls(c), enabled(b) {}

    bool match(HWND wnd) const;

    bool load(ef::Win::RegKeyH& key, int i);
    bool save(ef::Win::RegKeyH& key, int i) const;
    static void remove(ef::Win::RegKeyH& key, int i);
};


// Simple scalar option.
// Manages its own range and UI interaction.
//
template <typename T>
struct ScalarOption {
    T value, minV, maxV, step;
    ScalarOption(T value_, T min_, T max_, T step_ = T(1)) : 
    value(value_), minV(min_), maxV(max_), step(step_)
    {
        if (maxV < minV)
            maxV = minV;
        value = clamp(value);
    }

    T clamp(T n) const
    {
        return n < minV ? minV : n > maxV ? maxV : n;
    }

    bool inRange(T t) const
    {
        return minV <= t && t <= maxV;
    }

    ScalarOption& operator=(T t)
    {
        value = clamp(t);
        return *this;
    }

    bool operator!=(const ScalarOption& other) const
    {
        return value != other.value;
    }

    // get value from ctrl (use min val on error)
    // making sure it's in range
    T getUI(HWND wnd, int id)
    {
        if (!std::numeric_limits<T>::is_integer) {
            // only integers are supported for now
            return minV;
        }

        BOOL xlated;
        T t = static_cast<T>(GetDlgItemInt(wnd, id, &xlated, 
            std::numeric_limits<T>::is_signed));
        if (!xlated || t < minV)
            t = minV;
        else if (t > maxV)
            t = maxV;

        return t;
    }

    // Check control for out-of-range values.
    // If clamp is false, show warning and move focus to control.
    // If clamp is true, set control to a proper value.
    // Return whether final value is valid.
    //
    bool validateUI(HWND wnd, int id, bool clampValue)
    {
        if (!std::numeric_limits<T>::is_integer) {
            // only integers are supported for now
            return false;
        }

        const bool isSigned = std::numeric_limits<T>::is_signed;
        BOOL xlated;
        T t = static_cast<T>(GetDlgItemInt(wnd, id, &xlated, isSigned));
        if (xlated && inRange(t))
            return true;

        if (clampValue) {
            T newValue = clamp(t);
            SetDlgItemInt(wnd, id, static_cast<UINT>(newValue), isSigned);
            return true;
        }
        else {
            // report error
            HWND prevSib = GetWindow(GetDlgItem(wnd, id), GW_HWNDPREV);
            std::wstring label = Util::Text::remAccel(ef::Win::WndH(prevSib).getText());
            Util::Res::ResStr str(IDS_WRN_UIRANGE, 256, 
                DWORD_PTR(label.c_str()), DWORD(minV), DWORD(maxV));
            Util::App::warning(wnd, str);
            SetFocus(GetDlgItem(wnd, id));
            return false;
        }
    }

};

typedef ScalarOption<int>        IntOption;
typedef std::vector<AutoPinRule> AutoPinRules;


// Program options.
// Provides default values and manages persistence.
//
class Options {
public:
    // pins
    COLORREF      pinClr;
    IntOption     trackRate;
    bool          dblClkTray;
    bool          runOnStartup;
    // hotkeys
    bool          hotkeysOn;
    HotKey        hotEnterPin, hotTogglePin;
    // autopin
    bool          autoPinOn;
    AutoPinRules  autoPinRules;
    IntOption     autoPinDelay;
    // lang
    std::wstring  uiFile;     // empty means built-in exe resources
    std::wstring  helpFile;   // empty defaults to 'deskpins.chm'

    Options();
    ~Options();

    bool save() const;
    bool load();

protected:
    // constants
    static const HKEY   HKCU;

    static LPCWSTR REG_PATH_EF;
    static LPCWSTR REG_APR_SUBPATH;
    static LPCWSTR REG_PATH_RUN;

    static LPCWSTR REG_PINCLR;
    static LPCWSTR REG_POLLRATE;
    static LPCWSTR REG_TRAYDBLCLK;
    static LPCWSTR REG_AUTOPINON;
    static LPCWSTR REG_AUTOPINDELAY;
    static LPCWSTR REG_AUTOPINCOUNT;
    static LPCWSTR REG_AUTOPINRULE;
    static LPCWSTR REG_HOTKEYSON;
    static LPCWSTR REG_HOTNEWPIN;
    static LPCWSTR REG_HOTTOGGLEPIN;
    static LPCWSTR REG_LCLUI;
    static LPCWSTR REG_LCLHELP;

    // utilities
    bool REGOK(DWORD err) { return err == ERROR_SUCCESS; }
};


class WindowCreationMonitor;


// Used by options dialog to pass data to the various tabs.
//
struct OptionsPropSheetData {
    Options& opt;
    WindowCreationMonitor& winCreMon;
};
