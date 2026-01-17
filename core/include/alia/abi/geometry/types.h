#ifndef ALIA_GEOMETRY_TYPES_H
#define ALIA_GEOMETRY_TYPES_H

#include <alia/abi/base.h>

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

typedef struct alia_insets
{
    float left;
    float right;
    float top;
    float bottom;
} alia_insets;

typedef struct alia_affine2
{
    float a, b, c, d, tx, ty;
} alia_affine2;

ALIA_EXTERN_C_END

#endif /* ALIA_GEOMETRY_TYPES_H */
