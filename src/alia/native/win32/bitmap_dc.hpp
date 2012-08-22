#ifndef ALIA_NATIVE_WIN32_BITMAP_DC_HPP
#define ALIA_NATIVE_WIN32_BITMAP_DC_HPP

#include <alia/geometry.hpp>
#include <alia/native/win32/windows.hpp>

#pragma comment (lib, "gdi32.lib")

namespace alia { namespace native {

class bitmap_dc
{
 public:
    bitmap_dc();
    ~bitmap_dc();

    // DC is valid before calling create()
    HDC dc() const { return dc_; }

    // create the bitmap and bind it to the DC
    void create(vector<2,int> const& size, uint8_t initial_intensity);
    void create(vector<2,int> const& size);

    uint8_t const* pixels() const { return static_cast<uint8_t const*>(px_); }

 private:
    void* px_;
    HDC dc_;
    HBITMAP bitmap_;
};

}}

#endif
