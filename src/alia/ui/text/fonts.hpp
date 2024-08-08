#ifndef ALIA_UI_TEXT_FONTS_HPP
#define ALIA_UI_TEXT_FONTS_HPP

#include <string>

#ifdef _WIN32
#pragma warning(push, 0)
#endif

#include <include/core/SkFont.h>
#include <include/core/SkFontMgr.h>

#ifdef _WIN32
#pragma warning(pop)
#endif

namespace alia {

sk_sp<SkFontMgr>
get_skia_font_manager();

SkFont&
get_font(std::string const& name, SkScalar size);

} // namespace alia

#endif
