#ifndef ALIA_NATIVE_WIN32_THEMED_HPP
#define ALIA_NATIVE_WIN32_THEMED_HPP

#include <alia/native/win32/uxtheme.hpp>
#include <alia/forward.hpp>
#include <alia/vector.hpp>
#include <alia/image.hpp>

// utilities for interfacing with uxtheme.dll

namespace alia { namespace native {

vector2i get_themed_widget_size(theme_set* themes, HTHEME theme, int part,
    int state);

void get_themed_border_size(vector2i* top_left, vector2i* bottom_right,
    theme_set* themes, HTHEME theme, int part, int state);

void create_themed_widget_image(image<rgba8>* image, theme_set* themes,
    HTHEME theme, int part, int state);

void create_themed_widget_image(image<rgba8>* image, theme_set* themes,
    HTHEME theme, int part, int state, vector2i const& widget_size);

void create_themed_widget_image(image<rgb8>* image, theme_set* themes,
    HTHEME theme, int part, int state);

void create_themed_widget_image(image<rgb8>* image, theme_set* themes,
    HTHEME theme, int part, int state, vector2i const& widget_size);

struct themed_widget_creator
{
    theme_set* themes;
    HTHEME theme;
    int part;
    void operator()(image<rgba8>* image, int state) const
    {
        create_themed_widget_image(image, themes, theme, part, state);
    }
};

struct themed_sized_widget_creator
{
    theme_set* themes;
    HTHEME theme;
    int part;
    vector2i size;
    void operator()(image<rgba8>* image, int state) const
    {
        create_themed_widget_image(image, themes, theme, part, state, size);
    }
};

font get_themed_font(theme_set* themes, int font_id, font const& fallback);

}}

#endif
