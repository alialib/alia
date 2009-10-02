#ifndef ALIA_NATIVE_WIN32_FONT_INFO_HPP
#define ALIA_NATIVE_WIN32_FONT_INFO_HPP

#include <alia/font.hpp>
#include <alia/native/win32/windows.hpp>

namespace alia { namespace native {

struct font_info : alia::font::system_info
{
    font_info(LOGFONTW const& logfont) : logfont(logfont) {}

    system_info* adjust_style(unsigned old_style, unsigned new_style) const;
    system_info* adjust_size(float size) const;

    LOGFONTW logfont;
};

font make_font(LOGFONTW const& logfont);

}}

#endif
