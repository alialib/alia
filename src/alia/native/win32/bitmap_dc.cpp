#ifdef WIN32

#include <alia/native/win32/bitmap_dc.hpp>
#include <alia/exception.hpp>

namespace alia { namespace native {

bitmap_dc::bitmap_dc()
{
    px_ = NULL;
    dc_ = NULL;
    bitmap_ = NULL;

    dc_ = CreateCompatibleDC(NULL);
    if (dc_ == NULL)
        throw exception("bitmap_dc: CreateCompatibleDC() failed.");
}

void bitmap_dc::create(vector2i const& size, uint8 initial_intensity)
{
    create(size);
    memset(px_, initial_intensity, product(size) * 4);
}

void bitmap_dc::create(vector2i const& size)
{
    BITMAPINFO bmi;
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size[0];
    bmi.bmiHeader.biHeight = -size[1];
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;
    bmi.bmiHeader.biClrUsed = 0;
    bmi.bmiHeader.biClrImportant = 0;

    bitmap_ = CreateDIBSection(dc_, &bmi, DIB_RGB_COLORS, &px_, NULL, 0);
    if (bitmap_ == NULL)
        throw exception("bitmap_dc: CreateDIBSection() failed.");
    if (!SelectObject(dc_, bitmap_))
        throw exception("bitmap_dc: SelectObject() failed.");
}

bitmap_dc::~bitmap_dc()
{
    if (bitmap_ != NULL)
        DeleteObject(bitmap_);
    if (dc_ != NULL)
        DeleteDC(dc_);
}

}}

#endif
