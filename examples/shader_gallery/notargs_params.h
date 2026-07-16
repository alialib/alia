// Shared POD layout for the notargs effect params block.
// Included from Slang (GPU cbuffer) and C++ (CPU upload blob).
//
// Packing: consecutive floats in groups of four match HLSL cbuffer / std140
// float4 packing. Keep fields as float (no bool/int) so both sides agree.
// Size must stay a multiple of 16 bytes.

#ifndef NOTARGS_PARAMS_H
#define NOTARGS_PARAMS_H

struct notargs_effect_params
{
    float zoom;
    float t;
    float curl;
    float thickness;

    float cosx_factor;
    float cosy_factor;
    float siny_factor;
    float sinz_factor;

    float normalize_rays;
    float step_scaling;
    float iterations;
    float invert;

    float color_r;
    float color_g;
    float color_b;
    float color_factor;

    float sine_factor;
    float depth_scale;
    float pad0;
    float pad1;
};

#endif /* NOTARGS_PARAMS_H */
