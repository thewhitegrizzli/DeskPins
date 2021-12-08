#include "stdafx.h"
#include "util.h"
#include "pinshape.h"
#include "resource.h"


PinShape::PinShape() : bmp(0), rgn(0)
{
    sz.cx = sz.cy = 1;
}


PinShape::~PinShape()
{
    DeleteObject(bmp);
    DeleteObject(rgn);
}


bool PinShape::initImage(COLORREF clr)
{
    if (bmp) {
        DeleteObject(bmp);
        bmp = 0;
    }

    COLORREF clrMap[][2] = {
        { Util::Clr::white,  Util::Clr::light(clr) }, 
        { Util::Clr::silver, clr        }, 
        { Util::Clr::gray,   Util::Clr::dark(clr)  }
    };
    return (bmp = LoadBitmap(app.inst, MAKEINTRESOURCE(IDB_PIN)))
        && Util::Gfx::remapBmpColors(bmp, clrMap, ARRSIZE(clrMap));
}


bool PinShape::initShape()
{
    if (rgn) {
        DeleteObject(rgn);
        rgn = 0;
        sz.cx = sz.cy = 1;
    }

    if (HBITMAP bmp = LoadBitmap(app.inst, MAKEINTRESOURCE(IDB_PIN))) {
        if (!Util::Gfx::getBmpSize(bmp, sz))
            sz.cx = sz.cy = 1;
        rgn = ef::Win::RgnH::create(bmp, RGB(255,0,255));
        DeleteObject(bmp);
    }

    return rgn != 0;
}
