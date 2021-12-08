#pragma once

#define STRICT
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers


// use conservative settings
#undef _WIN32_WINNT
#define WINVER        0x0500
// PropertySheet fails on Win95 when passed >IE3 structure sizes
// with ERROR_CALL_NOT_IMPLEMENTED (120), and since we don't need
// the extra functionality, we limit ourselves to IE3
#define _WIN32_IE     0x0300
#define _RICHEDIT_VER 0x0100

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <windowsx.h>   // GET_X/Y_LPARAM
#include <prsht.h>
#include <commdlg.h>
#include <process.h>

#include <cstdlib>      // abs
#include <string>
#include <vector>
#include <memory>       // auto_ptr
#include <limits>

#include <boost/utility.hpp>
#include <boost/scoped_array.hpp>
#include <boost/foreach.hpp>

#include <ef/std/Common.hpp>
#include <ef/std/ClrConv.hpp>
#include <ef/std/Path.hpp>
#include <ef/std/string.hpp>

#include <ef/Win/Common.hpp>
#include <ef/Win/CustomControls/LinkCtrl.hpp>
#include <ef/Win/FileFinder.hpp>
#include <ef/Win/FileH.hpp>
#include "ef/Win/GdiObjH.hpp"
#include "ef/Win/htmlhelp.hpp"
#include "ef/Win/Nearest16BppColor.hpp"
#include "ef/Win/PrevInstance.hpp"
#include "ef/Win/RegKeyH.hpp"
#include "ef/Win/Ver.hpp"
#include "ef/Win/WinHelper.hpp"  // deprecated, but we need getLastErrorStr()
#include "ef/Win/WinSys.hpp"
#include <ef/Win/WndH.hpp>
#include <ef/Win/ModuleH.hpp>
