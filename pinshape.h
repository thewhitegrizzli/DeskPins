#pragma once


// Pin image and region.
// Used by pin windows to shape and paint themselves.
//
class PinShape : boost::noncopyable {
public:
    PinShape();
    ~PinShape();

    bool initShape();
    bool initImage(COLORREF clr);

    HBITMAP getBmp() const { return bmp;  }
    HRGN    getRgn() const { return rgn;  }
    int     getW()   const { return sz.cx; }
    int     getH()   const { return sz.cy; }

protected:
    HBITMAP bmp;
    HRGN    rgn;
    SIZE    sz;
};
