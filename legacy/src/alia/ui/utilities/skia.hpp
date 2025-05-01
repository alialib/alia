#ifndef ALIA_UI_UTILITIES_SKIA_HPP
#define ALIA_UI_UTILITIES_SKIA_HPP

#include <alia/ui/color.hpp>
#include <alia/ui/layout/specification.hpp>

#ifdef _WIN32
#pragma warning(push, 0)
#endif

#include <include/core/SkColor.h>
#include <include/core/SkRect.h>
#include <include/core/SkScalar.h>

#ifdef _WIN32
#pragma warning(pop)
#endif

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

inline SkRect
as_skrect(layout_box const& box)
{
    SkRect rect;
    rect.fLeft = box.corner[0];
    rect.fRight = box.corner[0] + box.size[0];
    rect.fTop = box.corner[1];
    rect.fBottom = box.corner[1] + box.size[1];
    return rect;
}

inline SkColor
as_skcolor(rgba8 color)
{
    return SkColorSetARGB(color.a, color.r, color.g, color.b);
}

inline SkColor
as_skcolor(rgb8 color)
{
    return SkColorSetARGB(0xff, color.r, color.g, color.b);
}

} // namespace alia

#endif
