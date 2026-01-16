#pragma once

#include <alia/abi/geometry/box_api.h>

#include <alia/raw/geometry/types.hpp>

namespace alia { namespace raw {

static inline box
box_make(vec2f min, vec2f size)
{
    return alia_box_make(min, size);
}

static inline bool
box_equal(box a, box b)
{
    return alia_box_equal(a, b);
}

static inline bool
operator==(box a, box b)
{
    return alia_box_equal(a, b);
}

static inline bool
operator!=(box a, box b)
{
    return !(a == b);
}

static inline box
box_expand(box box, vec2f v)
{
    return alia_box_expand(box, v);
}

static inline box
box_shrink(box box, vec2f v)
{
    return alia_box_shrink(box, v);
}

static inline box
box_outset(box box, insets i)
{
    return alia_box_outset(box, i);
}

static inline box
box_inset(box box, insets i)
{
    return alia_box_inset(box, i);
}

static inline bool
box_contains(box box, vec2f p)
{
    return alia_box_contains(box, p);
}

}} // namespace alia::raw
