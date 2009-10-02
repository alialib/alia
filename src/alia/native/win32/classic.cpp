#ifdef WIN32

#include <alia/native/win32/classic.hpp>
#include <alia/native/win32/widget_image.hpp>
#include <alia/native/win32/font.hpp>

namespace alia { namespace native {

unsigned get_classic_state(widget_state state)
{
    if ((state & widget_states::DISABLED) != 0)
    {
        return DFCS_INACTIVE;
    }
    else
    {
        switch (state & widget_states::PRIMARY_STATE_MASK)
        {
         case widget_states::HOT:
            return DFCS_HOT;
            break;
         case widget_states::DEPRESSED:
            return DFCS_PUSHED;
            break;
         default:
            return 0;
        }
    }
}

struct frame_control_painter
{
    unsigned type, state;
    RECT* rect;
    void operator()(HDC dc) const
    {
        DrawFrameControl(dc, rect, type, state);
    }
};
void create_classic_widget_image(image<rgba8>* image,
    vector2i const& size, unsigned type, unsigned state)
{
    RECT rect = { 0, 0, size[0], size[1] };
    frame_control_painter p;
    p.type = type;
    p.state = state;
    p.rect = &rect;
    capture_image(image, size, p);
}

static void checker_fill(image_view<rgb8>& v, rgb8 const& color0,
    rgb8 const& color1)
{
    for (int y = 0; y < v.size[1]; ++y)
    {
        bool odd = (y & 1) != 0;
        rgb8 const& c0 = odd ? color1 : color0;
        rgb8 const& c1 = odd ? color0 : color1;
        rgb8* end_i = get_row_end(v, y);
        for (rgb8* i = get_row_begin(v, y); i != end_i; ++i)
        {
            *i = c0;
            ++i;
            if (i == end_i)
                break;
            *i = c1;
        }
    }
}

void create_classic_scrollbar_background_image(image<rgb8>* image,
    vector2i const& size, int state)
{
    bool invert = (state & 1) != 0;
    bool odd = (state & 2) != 0;

    COLORREF scrollbar_color, face_color, window_color, highlight_color;

    scrollbar_color = GetSysColor(COLOR_SCROLLBAR);
    face_color = GetSysColor(COLOR_3DFACE);      
    window_color = GetSysColor(COLOR_WINDOW);
    highlight_color = GetSysColor(COLOR_3DHILIGHT);

    create_image(*image, size);

    if (face_color != scrollbar_color && window_color != scrollbar_color)
    {
        if (invert)
            scrollbar_color ^= 0x00ffffff;
        rgb8 c = win32_color_to_rgb8(scrollbar_color);
        alia_foreach_pixel(image->view, rgb8, i, i = c)
    }
    else
    {
        if (invert)
        {
            highlight_color ^= 0x00ffffff;
            face_color ^= 0x00ffffff;
        }
        checker_fill(image->view,
            win32_color_to_rgb8(odd ? face_color : highlight_color),
            win32_color_to_rgb8(odd ? highlight_color : face_color));
    }
}

struct edge_painter
{
    unsigned edge, flags;
    RECT* rect;
    void operator()(HDC dc) const
    {
        DrawEdge(dc, rect, edge, flags);
    }
};
void create_classic_thumb_image(image<rgba8>* image,
    vector2i const& size, unsigned edge, unsigned flags)
{
    RECT rect = { 0, 0, size[0], size[1] };
    edge_painter p;
    p.edge = edge;
    p.flags = flags;
    p.rect = &rect;
    capture_image(image, size, p);
}

void create_classic_slider_track_image(image<rgba8>* image,
    vector2i const& size)
{
    RECT rect = { 0, 0, size[0], size[1] };
    struct painter
    {
        RECT* rect;
        void operator()(HDC dc) const
        {
            RECT r = *rect;
            DrawEdge(dc, &r, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
            FillRect(dc, &r, HBRUSH(GetStockObject(GRAY_BRUSH)));
        }
    };
    painter p;
    p.rect = &rect;
    capture_image(image, size, p);
}

struct focus_rect_painter
{
    RECT* rect;
    void operator()(HDC dc) const
    {
        DrawFocusRect(dc, rect);
    }
};
void create_focus_rect_image(image<rgba8>* image, vector2i const& size)
{
    RECT rect = { 0, 0, size[0], size[1] };
    focus_rect_painter p;
    p.rect = &rect;
    capture_image(image, size, p);
}

font get_font(int font_id, font const& fallback)
{
    HFONT hfont = HFONT(GetStockObject(font_id));
    if (!hfont)
        return fallback;

    LOGFONTW logfont;
    if (!GetObjectW(hfont, sizeof(logfont), &logfont))
        return fallback;

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
