#ifdef WIN32

#include <alia/native/font_provider.hpp>
#include <alia/native/win32/bitmap_dc.hpp>
#include <alia/native/win32/widget_image.hpp>

namespace alia { namespace native {

struct win_font
{
    win_font(alia::font const& f)
    {
        char const* name = f.name.empty() ? "Tahoma" : f.name.c_str();
        font_ = CreateFont(
            -int(f.size + 0.5),
            0,
            0,
            0,
            is_bold(f) ? FW_BOLD : FW_NORMAL,
            is_italic(f) ? TRUE : FALSE,
            is_underlined(f) ? TRUE : FALSE,
            is_strikethrough(f) ? TRUE : FALSE,
            ANSI_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            name);

        if (font_ == NULL)
            throw exception("ascii_text_renderer: CreateFont() failed");
    }

    HFONT handle() const { return font_; }

    ~win_font()
    {
        DeleteObject(font_);
    }

 private:
    HFONT font_;
};

struct ascii_table_painter
{
    HFONT font;
    rgba8 text_color, bg_color;
    vector<2,int> image_size;
    vector<2,int> cell_size;
    int overhang;
    void operator()(HDC dc) const
    {
        SelectObject(dc, font);

        if (bg_color.a == 0xff)
        {
            SetBkColor(dc, RGB(bg_color.r, bg_color.g, bg_color.b));
            RECT rect;
            rect.left = 0;
            rect.top = 0;
            rect.right = image_size[0];
            rect.bottom = image_size[1];
            ExtTextOut(dc, 0, 0, ETO_OPAQUE, &rect, "", 0, 0);
        }
        else
            SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(text_color.r, text_color.g, text_color.b));

        for (int i = 0; i < 16; ++i)
        {
            for (int j = 0; j < 16; ++j)
            {
                char c = char(i * 16 + j);
                TextOut(dc, j * cell_size[0] + overhang,
                    i * cell_size[1], &c, 1);
            }
        }
    }
};

template<class Painter>
void capture_opaque_image(image<rgba8>* image, vector<2,int> const& size,
    Painter const& painter)
{
    assert(size[0] != 0 && size[1] != 0);
    bitmap_dc dc;
    dc.create(size);
    painter(dc.dc());
    create_image(*image, size);
    uint8_t const* p = dc.pixels();
    alia_foreach_pixel(image->view, rgba8, i,
        i.b = *p++;
        i.g = *p++;
        i.r = *p++;
        i.a = 0xff;
        ++p;
    )
};

void create_ascii_font_image(ascii_font_image* img,
    font const& font, rgba8 text_color, rgba8 bg_color)
{
    bitmap_dc dc;

    win_font wf(font);

    SelectObject(dc.dc(), wf.handle());

    TEXTMETRIC tm;
    GetTextMetrics(dc.dc(), &tm);

    int overhang = tm.tmOverhang;

    ABC widths[256];
    if (GetCharABCWidths(dc.dc(), 0, 255, widths))
    {
        // GetCharABCWidths succeeded. Copy results.
        for (int i = 0; i < 256; ++i)
        {
            img->advance[i] = widths[i].abcA + widths[i].abcB + widths[i].abcC;
            // The overhang value returned from GetTextMetrics() seems too
            // small in some cases, so check it and correct it here.
            if (-widths[i].abcA > overhang)
                overhang = -widths[i].abcA;
            if (-widths[i].abcC > overhang)
                overhang = -widths[i].abcC;
            img->trailing_space[i] = widths[i].abcC;
        }
    }
    else
    {
        // GetCharABCWidths fails on TrueType fonts, so fall back to this.
        INT widths[256];
        if (!GetCharWidth32(dc.dc(), 0, 255, widths))
        {
            throw exception(font.name + ": failed to query character widths");
        }
        for (int i = 0; i < 256; ++i)
        {
            img->advance[i] = int(widths[i]);
            img->trailing_space[i] = 0;
        }
    }
    img->advance['\n'] = img->advance['\r'] = 0;
    img->trailing_space['\n'] = img->trailing_space['\r'] = 0;

    int max_width = 0;
    for (int i = 0; i < 256; ++i)
    {
        if (img->advance[i] > max_width)
            max_width = img->advance[i];
    }

    vector<2,int> cell_size =
        make_vector<int>(max_width + overhang * 2, tm.tmHeight);
    img->cell_size = cell_size;
    vector<2,int> image_size = cell_size * 16;

    ascii_table_painter painter;
    painter.font = wf.handle();
    painter.text_color = text_color;
    painter.bg_color = bg_color;
    painter.image_size = image_size;
    painter.cell_size = cell_size;
    painter.overhang = overhang;

    if (bg_color.a == 0xff)
        capture_opaque_image(&img->image, image_size, painter);
    else
        capture_image(&img->image, image_size, painter);

    font_metrics& metrics = img->metrics;
    metrics.height = tm.tmHeight;
    metrics.ascent = tm.tmAscent;
    metrics.descent = tm.tmDescent;
    metrics.average_width = tm.tmAveCharWidth;
    metrics.row_gap =
        int(tm.tmHeight + tm.tmExternalLeading + 0.5 - tm.tmHeight);
    metrics.overhang = overhang;

    img->font = font;
    img->text_color = text_color;
    img->bg_color = bg_color;
}

}}

#endif
