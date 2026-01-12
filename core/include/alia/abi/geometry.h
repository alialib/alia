#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct alia_vec2
{
    float x, y;
} alia_vec2;

typedef struct alia_box
{
    alia_vec2 min;
    alia_vec2 size;
} alia_box;

typedef struct alia_rect
{
    alia_vec2 min;
    alia_vec2 max;
} alia_rect;

#ifdef __cplusplus
}
#endif
