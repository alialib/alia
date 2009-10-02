#ifdef WIN32

#include <alia/native/win32/themed.hpp>
#include <alia/native/win32/font.hpp>
#include <alia/native/win32/widget_image.hpp>
#include <alia/surface.hpp>

namespace alia { namespace native {

vector2i get_themed_widget_size(theme_set* themes, HTHEME theme, int part,
    int state)
{
    SIZE part_size;
    themes->dll.GetThemePartSize(theme, NULL, part, state, NULL, TS_TRUE,
        &part_size);

    return vector2i(part_size.cx, part_size.cy);
}

void get_themed_border_size(vector2i* top_left, vector2i* bottom_right,
    theme_set* themes, HTHEME theme, int part, int state)
{
    RECT bounding_rect = { 0, 0, 200, 200 };
    RECT content_rect;

    themes->dll.GetThemeBackgroundContentRect(theme, NULL, part, state,
        &bounding_rect, &content_rect);

    (*top_left)[0] = content_rect.left - bounding_rect.left;
    (*top_left)[1] = content_rect.top - bounding_rect.top;
    (*bottom_right)[0] = bounding_rect.right - content_rect.right;
    (*bottom_right)[1] = bounding_rect.bottom - content_rect.bottom;
}

void create_themed_widget_image(image<rgba8>* image, theme_set* themes,
    HTHEME theme, int part, int state)
{
    SIZE part_size;
    themes->dll.GetThemePartSize(theme, NULL, part, state, NULL, TS_TRUE,
        &part_size);

    create_themed_widget_image(image, themes, theme, part, state,
        vector2i(part_size.cx, part_size.cy));
}

struct themed_widget_painter
{
    theme_set* themes;
    HTHEME theme;
    int part, state;
    RECT* rect;
    void operator()(HDC dc) const
    {
        themes->dll.DrawThemeBackground(theme, dc, part, state, rect, NULL);
    }
};

void create_themed_widget_image(image<rgba8>* image, theme_set* themes,
    HTHEME theme, int part, int state, vector2i const& size)
{
    RECT rect = { 0, 0, size[0], size[1] };

    themed_widget_painter p;
    p.themes = themes;
    p.theme = theme;
    p.part = part;
    p.state = state;
    p.rect = &rect;

    capture_image(image, size, p);
}

void create_themed_widget_image(image<rgb8>* image, theme_set* themes,
    HTHEME theme, int part, int state)
{
    SIZE part_size;
    themes->dll.GetThemePartSize(theme, NULL, part, state, NULL, TS_TRUE,
        &part_size);

    create_themed_widget_image(image, themes, theme, part, state,
        vector2i(part_size.cx, part_size.cy));
}

void create_themed_widget_image(image<rgb8>* image, theme_set* themes,
    HTHEME theme, int part, int state, vector2i const& size)
{
    RECT rect = { 0, 0, size[0], size[1] };

    themed_widget_painter p;
    p.themes = themes;
    p.theme = theme;
    p.part = part;
    p.state = state;
    p.rect = &rect;

    capture_image(image, size, p);
}

font get_themed_font(theme_set* themes, int font_id, font const& fallback)
{
    // It seems any theme will work for this.
    HTHEME theme = themes->button;

    LOGFONTW logfont;
    if (themes->dll.GetThemeSysFont(theme, font_id, &logfont) != S_OK)
    {
        return fallback;
    }

    try
    {
        return make_font(logfont);
    }
    catch (...)
    {
        return fallback;
    }
}

}}

#endif
