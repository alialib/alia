#include <alia/ui/text/fonts.hpp>

#include <map>

#if defined(SK_BUILD_FOR_WIN) && defined(SK_FONTMGR_DIRECTWRITE_AVAILABLE)
#include "include/ports/SkTypeface_win.h"
#endif

#if defined(SK_BUILD_FOR_ANDROID) && defined(SK_FONTMGR_ANDROID_AVAILABLE)
#include "include/ports/SkFontMgr_android.h"
#include "src/ports/SkTypeface_FreeType.h"
#endif

#if defined(SK_FONTMGR_CORETEXT_AVAILABLE)                                    \
    && (defined(SK_BUILD_FOR_IOS) || defined(SK_BUILD_FOR_MAC))
#include "include/ports/SkFontMgr_mac_ct.h"
#endif

#if defined(SK_FONTMGR_FONTCONFIG_AVAILABLE)
#include "include/ports/SkFontMgr_fontconfig.h"
#endif

namespace alia {

namespace {

sk_sp<SkFontMgr>
create_skia_font_manager()
{
#if defined(SK_BUILD_FOR_ANDROID) && defined(SK_FONTMGR_ANDROID_AVAILABLE)
    return SkFontMgr_New_Android(
        nullptr, std::make_unique<SkFontScanner_FreeType>());
#elif defined(SK_BUILD_FOR_WIN) && defined(SK_FONTMGR_DIRECTWRITE_AVAILABLE)
    return SkFontMgr_New_DirectWrite();
#elif defined(SK_FONTMGR_CORETEXT_AVAILABLE)                                  \
    && (defined(SK_BUILD_FOR_IOS) || defined(SK_BUILD_FOR_MAC))
    return SkFontMgr_New_CoreText(nullptr);
#elif defined(SK_FONTMGR_FONTCONFIG_AVAILABLE)
    return SkFontMgr_New_FontConfig(nullptr);
#else
    return SkFontMgr::RefEmpty();
#endif
}

} // namespace

sk_sp<SkFontMgr>
get_skia_font_manager()
{
    static sk_sp<SkFontMgr> the_font_manager = create_skia_font_manager();
    return the_font_manager;
}

namespace {

sk_sp<SkTypeface>
get_typeface(std::string const& name)
{
    // TODO: configurable font dir
    static const std::string font_dir = "../";
    // TODO: more intelligent caching
    static std::map<std::string, sk_sp<SkTypeface>> cache;

    auto i = cache.find(name);
    if (i != cache.end())
    {
        return i->second;
    }
    else
    {
        auto font_path = font_dir + name + ".ttf";
        auto typeface
            = get_skia_font_manager()->makeFromFile(font_path.c_str());
        cache.insert(make_pair(name, typeface));
        return typeface;
    }
}

} // namespace

SkFont&
get_font(std::string const& name, SkScalar size)
{
    // TODO: more intelligent caching
    static std::map<std::pair<std::string, SkScalar>, SkFont> cache;

    auto cache_key = std::make_pair(name, size);
    auto iter = cache.find(cache_key);
    if (iter == cache.end())
    {
        auto typeface = get_typeface(name);
        auto font = SkFont(typeface, size);
        iter = cache.insert(make_pair(cache_key, font)).first;
    }
    return iter->second;
}

} // namespace alia
