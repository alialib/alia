#ifndef ALIA_NATIVE_WIN32_CLASSIC_HPP
#define ALIA_NATIVE_WIN32_CLASSIC_HPP

#include <alia/forward.hpp>
#include <alia/image.hpp>
#include <alia/widget_state.hpp>
#include <alia/color.hpp>
#include <alia/native/win32/windows.hpp>

namespace alia { namespace native {

unsigned get_classic_state(widget_state state);

void create_classic_widget_image(image<rgba8>* image,
    vector2i const& size, unsigned type, unsigned state);

struct classic_widget_creator
{
    vector2i size;
    int type, part;
    void operator()(image<rgba8>* image, int state) const
    {
        create_classic_widget_image(image, size, type, part | state);
    }
};

void create_classic_scrollbar_background_image(image<rgb8>* image,
    vector2i const& size, int state);

void create_classic_thumb_image(image<rgba8>* image,
    vector2i const& size, unsigned edge, unsigned flags);

void create_classic_slider_track_image(image<rgba8>* image,
    vector2i const& size);

inline rgb8 win32_color_to_rgb8(COLORREF c)
{
    rgb8 p;
    p.r = GetRValue(c);
    p.g = GetGValue(c);
    p.b = GetBValue(c);
    return p;
}

inline rgb8 get_sys_color(int color_id)
{
    COLORREF c = GetSysColor(color_id);
    return rgb8(GetRValue(c), GetGValue(c), GetBValue(c));
}

void create_focus_rect_image(image<rgba8>* image, vector2i const& size);

font get_font(int font_id, font const& fallback);

}}

#endif
