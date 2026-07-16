// Alia portable effect frame interface (Slang).
//
// Backends bind:
//   b0 / UBO binding 0: AliaEffectFrame (region + surface, 32 bytes)
//   b1 / UBO binding 1: Effect (per-effect user params; declared by the effect)
//
// Fragment entry: ps_main(float4 frag_coord : SV_Position) : SV_Target
//
// `frag_coord` is in Alia surface pixel space on D3D (top-left origin). On
// GLSL targets, use alia_effect_surface.y to flip gl_FragCoord as needed
// (see __target_switch in notargs.slang).

cbuffer AliaEffectFrame : register(b0)
{
    float4 alia_effect_region; // xy = min, zw = size (surface space)
    float4 alia_effect_surface; // xy = surface size in pixels
};
