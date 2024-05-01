#ifndef ALIA_UI_TEXT_FONTS_HPP
#define ALIA_UI_TEXT_FONTS_HPP

#include <string>

#include <include/core/SkFont.h>
#include <include/core/SkFontMgr.h>

namespace alia {

sk_sp<SkFontMgr>
get_skia_font_manager();

SkFont&
get_font(std::string const& name, SkScalar size);

} // namespace alia

#endif
