#include "stdafx.h"
#include "util.h"
#include "options.h"


const HKEY Options::HKCU = HKEY_CURRENT_USER;

LPCWSTR Options::REG_PATH_EF      = L"Software\\Elias Fotinis";
LPCWSTR Options::REG_APR_SUBPATH  = L"AutoPinRules";
LPCWSTR Options::REG_PATH_RUN     = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

LPCWSTR Options::REG_PINCLR       = L"PinColor";
LPCWSTR Options::REG_POLLRATE     = L"PollRate";
LPCWSTR Options::REG_TRAYDBLCLK   = L"TrayDblClk";
LPCWSTR Options::REG_AUTOPINON    = L"Enabled";
LPCWSTR Options::REG_AUTOPINDELAY = L"Delay";
LPCWSTR Options::REG_AUTOPINCOUNT = L"Count";
LPCWSTR Options::REG_AUTOPINRULE  = L"AutoPinRule%d";
LPCWSTR Options::REG_HOTKEYSON    = L"HotKeysOn";
LPCWSTR Options::REG_HOTNEWPIN    = L"HotKeyNewPin";
LPCWSTR Options::REG_HOTTOGGLEPIN = L"HotKeyTogglePin";
LPCWSTR Options::REG_LCLUI        = L"LocalizedUI";
LPCWSTR Options::REG_LCLHELP      = L"LocalizedHelp";


bool 
HotKey::load(ef::Win::RegKeyH& key, LPCWSTR val)
{
    DWORD dw;
    if (!key.getDWord(val, dw))
        return false;

    vk  = LOBYTE(dw);
    mod = HIBYTE(dw);

    if (!vk)  // oops
        mod = 0;

    return true;
}


bool 
HotKey::save(ef::Win::RegKeyH& key, LPCWSTR val) const
{
    return key.setDWord(val, MAKEWORD(vk, mod));
}


bool 
AutoPinRule::match(HWND wnd) const
{
    return enabled 
        && ef::wildimatch(ttl, ef::Win::WndH(wnd).getText()) 
        && ef::wildimatch(cls, ef::Win::WndH(wnd).getClassName());
}


class NumFlagValueName {
    const int num;
public:
    NumFlagValueName(int num) : num(num) {}
    std::wstring operator()(WCHAR flag) {
        WCHAR buf[20];
        wsprintf(buf, L"%d%c", num, flag);
        return buf;
    }
};


bool 
AutoPinRule::load(ef::Win::RegKeyH& key, int i)
{
    NumFlagValueName val(i);
    DWORD enabled_dw;
    if (!key.getString(val(L'D'), descr) || 
        !key.getString(val(L'T'), ttl) ||
        !key.getString(val(L'C'), cls) ||
        !key.getDWord(val(L'E'), enabled_dw))
    {
        return false;
    }
    enabled = enabled_dw != 0;
    return true;
}


bool 
AutoPinRule::save(ef::Win::RegKeyH& key, int i) const
{
    NumFlagValueName val(i);
    return key.setString(val(L'D'), descr) &&
           key.setString(val(L'T'), ttl) &&
           key.setString(val(L'C'), cls) &&
           key.setDWord(val(L'E'), enabled);
}


void 
AutoPinRule::remove(ef::Win::RegKeyH& key, int i)
{
    NumFlagValueName val(i);
    key.deleteValue(val(L'D'));
    key.deleteValue(val(L'T'));
    key.deleteValue(val(L'C'));
    key.deleteValue(val(L'E'));
}


Options::Options() : 
    pinClr(RGB(255,0,0)),
    trackRate(100,10,1000,10),
    dblClkTray(false),
    runOnStartup(false),
    hotkeysOn(true),
    hotEnterPin(App::HOTID_ENTERPINMODE, VK_F11, MOD_CONTROL),
    hotTogglePin(App::HOTID_TOGGLEPIN, VK_F12, MOD_CONTROL),
    autoPinOn(false),
    autoPinDelay(200,100,10000,50),
    helpFile(L"DeskPins.chm")    // init, in case key doesn't exist
{
    // set higher tracking rate for Win2K+ (higher WM_TIMER resolution)

    if (ef::Win::OsVer().major() >= HIWORD(ef::Win::OsVer::win2000))
        trackRate.value = 20;
}


Options::~Options()
{
}


bool 
Options::save() const
{
    std::wstring appKeyPath = std::wstring(REG_PATH_EF) + L'\\' + App::APPNAME;
    ef::Win::AutoRegKeyH key = ef::Win::RegKeyH::create(HKCU, appKeyPath);
    if (!key) return false;

    key.setDWord(REG_PINCLR, pinClr < 7 ? 0 : pinClr);   // HACK: see load()
    key.setDWord(REG_POLLRATE, trackRate.value);
    key.setDWord(REG_TRAYDBLCLK, dblClkTray);
    ef::Win::AutoRegKeyH runKey = ef::Win::RegKeyH::create(HKCU, REG_PATH_RUN);
    if (runKey) {
        if (runOnStartup)
            runKey.setString(App::APPNAME, ef::Win::ModuleH(0).getFileName());
        else
            runKey.deleteValue(App::APPNAME);
    }

    key.setDWord(REG_HOTKEYSON, hotkeysOn);
    hotEnterPin.save(key, REG_HOTNEWPIN);
    hotTogglePin.save(key, REG_HOTTOGGLEPIN);

    ef::Win::AutoRegKeyH apKey = ef::Win::RegKeyH::create(key, REG_APR_SUBPATH);
    if (apKey) {
        int oldCount;
        if (!apKey.getDWord(REG_AUTOPINCOUNT, reinterpret_cast<DWORD&>(oldCount)))
            oldCount = 0;
        apKey.setDWord(REG_AUTOPINON, autoPinOn);
        apKey.setDWord(REG_AUTOPINDELAY, autoPinDelay.value);
        apKey.setDWord(REG_AUTOPINCOUNT, autoPinRules.size());
        int n;
        for (n = 0; n < int(autoPinRules.size()); ++n)
            autoPinRules[n].save(apKey, n);
        // remove old left-over rules
        for (; n < oldCount; ++n)
            AutoPinRule::remove(apKey, n);
    }

    key.setString(REG_LCLUI, uiFile);
    key.setString(REG_LCLHELP, helpFile);

    return true;
}


bool
Options::load()
{
    std::wstring appKeyPath = std::wstring(REG_PATH_EF) + L'\\' + App::APPNAME;
    ef::Win::AutoRegKeyH key = ef::Win::RegKeyH::open(HKCU, appKeyPath);
    if (!key) return false;

    DWORD dw;
    std::wstring buf;

    if (key.getDWord(REG_PINCLR, dw)) {
        pinClr = COLORREF(dw & 0xFFFFFF);
        // HACK: Prior to version 1.3 the color was an index (0..6), 
        //       but from v1.3+ a full 24-bit clr value is used.
        //       To avoid interpreting the old setting as an RGB
        //       (and thus producing a nasty black clr)
        //       we have to translate the older index values into real RGB.
        //       Also, in save() we replace COLORREFs < 0x000007 with 0 (black)
        //       to get this straight (and hope no-one notices :D ).
        // FIXME: no longer needed
        if (pinClr > 0 && pinClr < 7) {
            using namespace Util::Clr;
            const COLORREF clr[6] = {red, yellow, lime, cyan, blue, magenta};
            pinClr = clr[pinClr-1];
        }
    }
    if (key.getDWord(REG_POLLRATE, dw) && trackRate.inRange(dw))
        trackRate = dw;
    if (key.getDWord(REG_TRAYDBLCLK, dw))
        dblClkTray = dw != 0;
    ef::Win::AutoRegKeyH runKey = ef::Win::RegKeyH::open(HKCU, REG_PATH_RUN);
    if (runKey)
        runOnStartup = runKey.getValueType(App::APPNAME) == REG_SZ;

    if (key.getDWord(REG_HOTKEYSON, dw))
        hotkeysOn = dw != 0;
    hotEnterPin.load(key, REG_HOTNEWPIN);
    hotTogglePin.load(key, REG_HOTTOGGLEPIN);

    ef::Win::AutoRegKeyH apKey = ef::Win::RegKeyH::open(key, REG_APR_SUBPATH);
    if (apKey) {
        if (apKey.getDWord(REG_AUTOPINON, dw))
            autoPinOn = dw != 0;
        if (apKey.getDWord(REG_AUTOPINDELAY, dw) && autoPinDelay.inRange(dw))
            autoPinDelay = dw;
        if (apKey.getDWord(REG_AUTOPINCOUNT, dw)) {
            for (int n = 0; n < int(dw); ++n) {
                AutoPinRule rule;
                if (rule.load(apKey, n))
                    autoPinRules.push_back(rule);
            }
        }
    }

    if (key.getString(REG_LCLUI, buf))
        uiFile = buf;
    if (key.getString(REG_LCLHELP, buf))
        helpFile = buf;

    if (helpFile.empty())
        helpFile = L"DeskPins.chm";

    return true;
}
