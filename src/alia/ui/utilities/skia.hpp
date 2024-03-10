#ifndef ALIA_UI_UTILITIES_SKIA_HPP
#define ALIA_UI_UTILITIES_SKIA_HPP

#include <alia/ui/layout/specification.hpp>

namespace alia {

// Cast a Skia scalar to layout scalar representing a size.
inline layout_scalar
skia_scalar_as_layout_size(SkScalar x)
{
    return x;
}
// Cast a Skia scalar to a layout scalar.
inline layout_scalar
skia_scalar_as_layout_scalar(SkScalar x)
{
    return x;
}
// Cast a layout scalar to a Skia scalar.
inline SkScalar
layout_scalar_as_skia_scalar(layout_scalar x)
{
    return x;
}

typedef vector<2, SkScalar> skia_vector;
typedef box<2, SkScalar> skia_box;

inline SkRect
as_skrect(skia_box const& box)
{
    SkRect rect;
    rect.fLeft = box.corner[0];
    rect.fRight = box.corner[0] + box.size[0];
    rect.fTop = box.corner[1];
    rect.fBottom = box.corner[1] + box.size[1];
    return rect;
}

} // namespace alia

#endif
