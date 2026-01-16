#ifndef ALIA_GEOMETRY_BOX_API_H
#define ALIA_GEOMETRY_BOX_API_H

#include <alia/abi/base/config.h>
#include <alia/abi/geometry/types.h>
#include <alia/abi/geometry/vec2_api.h>

ALIA_EXTERN_C_BEGIN

static inline alia_box
alia_box_make(alia_vec2f min, alia_vec2f size)
{
    return ALIA_BRACED_INIT(alia_box, min, size);
}

static inline bool
alia_box_equal(alia_box a, alia_box b)
{
    return alia_vec2f_equal(a.min, b.min) && alia_vec2f_equal(a.size, b.size);
}

static inline alia_box
alia_box_expand(alia_box box, alia_vec2f v)
{
    return alia_box_make(
        alia_vec2f_sub(box.min, v),
        alia_vec2f_add(box.size, alia_vec2f_scale(v, 2.0f)));
}

static inline alia_box
alia_box_shrink(alia_box box, alia_vec2f v)
{
    return alia_box_make(
        alia_vec2f_add(box.min, v),
        alia_vec2f_sub(box.size, alia_vec2f_scale(v, 2.0f)));
}

static inline alia_box
alia_box_outset(alia_box box, alia_insets i)
{
    alia_vec2f lt = alia_vec2f_make(i.left, i.top);
    alia_vec2f rb = alia_vec2f_make(i.right, i.bottom);
    return alia_box_make(
        alia_vec2f_sub(box.min, lt),
        alia_vec2f_add(box.size, alia_vec2f_add(lt, rb)));
}

static inline alia_box
alia_box_inset(alia_box box, alia_insets i)
{
    alia_vec2f lt = alia_vec2f_make(i.left, i.top);
    alia_vec2f rb = alia_vec2f_make(i.right, i.bottom);
    return alia_box_make(
        alia_vec2f_add(box.min, lt),
        alia_vec2f_sub(box.size, alia_vec2f_add(lt, rb)));
}

static inline bool
alia_box_contains(alia_box box, alia_vec2f p)
{
    return p.x >= box.min.x && p.x < box.min.x + box.size.x && p.y >= box.min.y
        && p.y < box.min.y + box.size.y;
}

ALIA_EXTERN_C_END

#endif /* ALIA_GEOMETRY_BOX_API_H */
