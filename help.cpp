#include "stdafx.h"
#include "util.h"
#include "help.h"
#include "resource.h"


bool Help::init(HINSTANCE inst, const std::wstring& fname) 
{
    hlpFile = L"";

    std::wstring path = ef::dirSpec(ef::Win::getModulePath(inst));
    if (path.empty())
        return false;

#if defined(DEBUG) || defined(_DEBUG)
    path += L"..\\help\\";
#endif

    hlpFile = path + fname;
    return true;
}


// if 'topic' is empty, use just the filename (goes to default topic)
// otherwise, append it to file spec
//   topic format is:  "::\someTopic.html"  or
//                     "::\someTopic.html#namedAnchor"
HWND Help::show(HWND wnd, const std::wstring& topic)
{
    if (hlpFile.empty()) {
        Util::App::error(wnd, Util::Res::ResStr(IDS_ERR_HELPMISSING));
        return 0;
    }

    return ef::Win::HTMLHelp::obj().dispTopic(wnd, (hlpFile + topic).c_str());
}
