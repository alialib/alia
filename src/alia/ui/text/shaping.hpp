#ifndef ALIA_UI_TEXT_SHAPING_HPP
#define ALIA_UI_TEXT_SHAPING_HPP

#include <vector>

#include <include/core/SkTextBlob.h>

namespace alia {

struct ShapeResult
{
    sk_sp<SkTextBlob> blob;
    std::vector<std::size_t> lineBreakOffsets;
    std::vector<SkRect> glyphBounds;
    int verticalAdvance;
    SkScalar width;
};

ShapeResult
Shape(
    const char* utf8Text, size_t textByteLen, const SkFont& font, float width);

} // namespace alia

#endif
