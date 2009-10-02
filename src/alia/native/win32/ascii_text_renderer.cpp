#ifdef WIN32

#include <alia/native/win32/ascii_text_renderer.hpp>
#include <alia/native/win32/bitmap_dc.hpp>
#include <alia/font.hpp>
#include <alia/native/win32/font.hpp>
#include <alia/exception.hpp>

namespace alia { namespace native {

struct win_font
{
    win_font(alia::font const& f)
    {
        font_info const* info = dynamic_cast<font_info const*>(f.get_info());
        if (info == NULL)
        {
            font_ = CreateFont(
                -int(f.get_size() + 0.5),
                0,
                0,
                0,
                f.is_bold() ? FW_BOLD : FW_NORMAL,
                f.is_italic() ? TRUE : FALSE,
                f.is_underlined() ? TRUE : FALSE,
                FALSE,
                ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY,
                DEFAULT_PITCH | FF_DONTCARE,
                f.get_name().c_str());
        }
        else
            font_ = CreateFontIndirectW(&info->logfont);

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

ascii_text_renderer::ascii_text_renderer(surface& surface,
    alia::font const& font)
  : widths_(new int [256])
{
    bitmap_dc dc;

    win_font wf(font);

    SelectObject(dc.get_dc(), wf.handle());

    TEXTMETRIC tm;
    GetTextMetrics(dc.get_dc(), &tm);

    // The overhang seems too small sometimes, so add an extra pixel.
    //int overhang = tm.tmOverhang + 1;

    //if (sizeof(INT) == sizeof(int))
    //{
    //    GetCharWidth32(dc.get_dc(), 0, 255, widths_.get());
    //}
    //else
    //{
    //    INT widths[256];
    //    GetCharWidth32(dc.get_dc(), 0, 255, widths);
    //    for (int i = 0; i < 256; ++i)
    //        widths_[i] = int(widths[i]);
    //}

    int overhang = 0;

    ABC widths[256];
    if (!GetCharABCWidths(dc.get_dc(), 0, 255, widths))
        throw exception("GetCharABCWidths() failed\n"); // TODO
    for (int i = 0; i < 256; ++i)
    {
        widths_[i] = widths[i].abcA + widths[i].abcB + widths[i].abcC;
        if (-widths[i].abcA > overhang)
            overhang = -widths[i].abcA;
        if (-widths[i].abcC > overhang)
            overhang = -widths[i].abcC;
    }
    widths_['\n'] = widths_['\r'] = 0;

    vector2i cell_size(tm.tmMaxCharWidth + overhang * 2, tm.tmHeight),
        image_size = cell_size * 16;
    dc.create(image_size, 0);

    SetBkColor(dc.get_dc(), RGB(0, 0, 0));
    SetTextColor(dc.get_dc(), RGB(255,255,255));

    for (int i = 0; i < 16; ++i)
    {
        for (int j = 0; j < 16; ++j)
        {
            char c = char(i * 16 + j);
            TextOut(dc.get_dc(), j * cell_size[0] + overhang,
                i * cell_size[1], &c, 1);
        }
    }

    create_image(image_, image_size);
    uint8 const* px = dc.get_pixels();
    alia_foreach_pixel(image_.view, uint8, i, i = *px; px += 4);

    font_metrics metrics;
    metrics.height = tm.tmHeight;
    metrics.ascent = tm.tmAscent;
    metrics.descent = tm.tmDescent;
    metrics.average_width = tm.tmAveCharWidth;
    metrics.row_gap = int((tm.tmHeight + tm.tmExternalLeading) *
        font.get_line_height() + 0.5) - tm.tmHeight;
    metrics.overhang = overhang;

    initialize(surface, font, &image_.view, widths_.get(), metrics);
}

}}

#endif
