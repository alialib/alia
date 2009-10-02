#ifndef ALIA_NATIVE_WIN32_BITMAP_DC_HPP
#define ALIA_NATIVE_WIN32_BITMAP_DC_HPP

#include <alia/vector.hpp>
#include <alia/typedefs.hpp>
#include <alia/native/win32/windows.hpp>

#pragma comment (lib, "gdi32.lib")

namespace alia { namespace native {

class bitmap_dc
{
 public:
    bitmap_dc();
    ~bitmap_dc();

    // DC is valid before calling create()
    HDC get_dc() const { return dc_; }

    // create the bitmap and bind it to the DC
    void create(vector2i const& size, uint8 initial_intensity);
    void create(vector2i const& size);

    uint8 const* get_pixels() const { return static_cast<uint8 const*>(px_); }

 private:
    void* px_;
    HDC dc_;
    HBITMAP bitmap_;
};

}}

#endif
