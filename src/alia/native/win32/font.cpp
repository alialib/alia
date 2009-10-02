#ifdef WIN32

#include <alia/native/win32/font.hpp>
#include <vector>
#include <alia/exception.hpp>

namespace alia { namespace native {

font::system_info* font_info::adjust_style(unsigned old_style,
    unsigned new_style) const
{
    font_info* new_info = new font_info(*this);

    if ((old_style & font::BOLD) != 0 && (new_style & font::BOLD) == 0)
    {
        new_info->logfont.lfWeight = FW_NORMAL;
    }
    else if ((old_style & font::BOLD) == 0 &&
        (new_style & font::BOLD) != 0)
    {
        new_info->logfont.lfWeight = FW_BOLD;
    }

    new_info->logfont.lfItalic = (new_style & font::ITALIC) != 0 ? 1 : 0;
    new_info->logfont.lfUnderline =
        (new_style & font::UNDERLINED) != 0 ? 1 : 0;

    return new_info;
}
font::system_info* font_info::adjust_size(float size) const
{
    font_info* new_info = new font_info(*this);
    new_info->logfont.lfHeight = -int(size + 0.5);
    return new_info;
}

font make_font(LOGFONTW const& logfont)
{
    int length = WideCharToMultiByte(CP_UTF8, 0, logfont.lfFaceName, -1,
        NULL, 0, NULL, NULL);
    if (length == 0)
        throw exception("WideCharToMultiByte failed");

    std::vector<char> name(length);
    if (WideCharToMultiByte(CP_UTF8, 0, logfont.lfFaceName, -1,
        &name[0], length, NULL, NULL) != length)
    {
        throw exception("WideCharToMultiByte failed");
    }

    return font(&name[0],
        float(logfont.lfHeight > 0 ? logfont.lfHeight : -logfont.lfHeight),
        boost::shared_ptr<font::system_info>(new font_info(logfont)),
        ((logfont.lfWeight >= FW_BOLD) ? font::BOLD : 0) |
        (logfont.lfItalic ? font::ITALIC : 0) |
        (logfont.lfUnderline ? font::UNDERLINED : 0));
}

}}

#endif
