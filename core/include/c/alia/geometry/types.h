#ifndef ALIA_GEOMETRY_TYPES_H
#define ALIA_GEOMETRY_TYPES_H

#include <alia/base/config.h>
#include <alia/base/types.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_vec2i
{
    int32_t x;
    int32_t y;
} alia_vec2i;

typedef struct alia_vec2f
{
    float x;
    float y;
} alia_vec2f;

typedef struct alia_box
{
    alia_vec2f min;
    alia_vec2f size;
} alia_box;

typedef struct alia_rect
{
    alia_vec2f min;
    alia_vec2f max;
} alia_rect;

ALIA_EXTERN_C_END

#endif /* ALIA_GEOMETRY_TYPES_H */
