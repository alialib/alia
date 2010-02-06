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
  , trailing_space_(new int [256])
{
    bitmap_dc dc;

    win_font wf(font);

    SelectObject(dc.get_dc(), wf.handle());

    TEXTMETRIC tm;
    GetTextMetrics(dc.get_dc(), &tm);

    int overhang = tm.tmOverhang;

    ABC widths[256];
    if (!GetCharABCWidths(dc.get_dc(), 0, 255, widths))
    {
        // GetCharABCWidths fails on TrueType fonts, so fall back to this.
        INT widths[256];
        if (!GetCharWidth32(dc.get_dc(), 0, 255, widths))
        {
            throw exception(font.get_name() +
                ": failed to query character widths");
        }
        for (int i = 0; i < 256; ++i)
        {
            widths_[i] = int(widths[i]);
            trailing_space_[i] = 0;
        }
    }
    else
    {
        for (int i = 0; i < 256; ++i)
        {
            widths_[i] = widths[i].abcA + widths[i].abcB + widths[i].abcC;
            // The overhang value returned from GetTextMetrics() seems too
            // small in some cases, so we check it here.
            if (-widths[i].abcA > overhang)
                overhang = -widths[i].abcA;
            if (-widths[i].abcC > overhang)
                overhang = -widths[i].abcC;
            trailing_space_[i] = widths[i].abcC;
        }
    }
    widths_['\n'] = widths_['\r'] = 0;
    trailing_space_['\n'] = trailing_space_['\r'] = 0;

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

    initialize(surface, font, &image_.view, widths_.get(),
        trailing_space_.get(), metrics);
}

}}

#endif
