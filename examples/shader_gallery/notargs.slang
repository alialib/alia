// notargs — gallery demo effect (Slang → DXBC / GLSL ES via alia_shader_builder).
#include "alia_effect_frame.slang"
#include "notargs_params.h"

cbuffer Effect : register(b1)
{
    notargs_effect_params params;
};

static const float tau = 6.28318530718;

float sdf(float3 p)
{
    float t = params.t;
    float curl = params.curl;
    float thickness = params.thickness;
    float cosx_factor = params.cosx_factor;
    float cosy_factor = params.cosy_factor;
    float siny_factor = params.siny_factor;
    float sinz_factor = params.sinz_factor;

    p.z -= t;
    float a = fmod(p.z * 0.1, tau) * curl;
    float cs = cos(a);
    float sn = sin(a);
    float2 xy = float2(cs * p.x - sn * p.y, sn * p.x + cs * p.y);
    p.xy = xy;
    return thickness
         - length(
               float2(cos(p.x) * cosx_factor, cos(p.y) * cosy_factor)
               + float2(sin(p.y) * siny_factor, sin(p.z) * sinz_factor));
}

[shader("fragment")]
float4 ps_main(float4 frag_coord : SV_Position) : SV_Target
{
    float2 corner = alia_effect_region.xy;
    float2 size = alia_effect_region.zw;
    float zoom = params.zoom;
    bool normalize_rays = params.normalize_rays > 0.5;
    float step_scaling = params.step_scaling;
    int iterations = int(params.iterations + 0.5);
    bool invert = params.invert > 0.5;
    float3 color = float3(params.color_r, params.color_g, params.color_b);
    float color_factor = params.color_factor;
    float sine_factor = params.sine_factor;
    float depth_scale = params.depth_scale;

    float2 frag;
    __target_switch
    {
    case hlsl:
        frag = frag_coord.xy;
        break;
    case glsl:
        // Map GL bottom-left fragment coords into Alia surface space.
        frag = float2(frag_coord.x, alia_effect_surface.y - frag_coord.y);
        break;
    }

    float2 half_size = size / 2.0;
    float2 uv = (frag - corner - half_size) / (zoom * half_size.y);

    float3 direction;
    if (normalize_rays)
        direction = normalize(float3(uv, 1.0)) * step_scaling;
    else
        direction = float3(uv, step_scaling);

    float distance_traveled = 0.0;
    for (int i = 0; i < iterations; ++i)
        distance_traveled += sdf(direction * distance_traveled);

    float3 p = direction * distance_traveled;
    float depth_factor = invert ? (length(p) * depth_scale / 120.0)
                                : (1.0 / (length(p) * depth_scale));
    float3 srgb = (sin(p) * sine_factor + color * color_factor) * depth_factor;

    // Convert to linear RGB.
    float3 low = srgb / 12.92;
    float3 high = pow((srgb + 0.055) / 1.055, 2.4);
    float3 linear = lerp(low, high, step(0.04045, srgb));

    return float4(linear, 1.0);
}
