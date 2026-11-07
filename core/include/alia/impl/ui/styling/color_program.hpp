#pragma once

#include <alia/abi/ui/styling/color_program.h>
#include <alia/abi/ui/styling/environment.h>

namespace alia {
namespace color {

// short aliases for core palette colors
inline constexpr alia_style_color_slot surface = ALIA_STYLE_COLOR_SURFACE;
inline constexpr alia_style_color_slot neutral = ALIA_STYLE_COLOR_NEUTRAL;
inline constexpr alia_style_color_slot text = ALIA_STYLE_COLOR_TEXT;
inline constexpr alia_style_color_slot well = ALIA_STYLE_COLOR_WELL;
inline constexpr alia_style_color_slot focus = ALIA_STYLE_COLOR_FOCUS;
inline constexpr alia_style_color_slot primary = ALIA_STYLE_COLOR_PRIMARY;
inline constexpr alia_style_color_slot secondary = ALIA_STYLE_COLOR_SECONDARY;
inline constexpr alia_style_color_slot tertiary = ALIA_STYLE_COLOR_TERTIARY;
inline constexpr alia_style_color_slot danger = ALIA_STYLE_COLOR_DANGER;

// maximum steps in a single expression chain during authoring
constexpr uint8_t color_program_expr_max_steps = 96;

// ordered list of program steps built by `|` expressions or `view`
struct color_program_expr
{
    alia_color_program_step steps[color_program_expr_max_steps]{};
    uint8_t count = 0;

    void
    push_step(alia_color_program_step step)
    {
        ALIA_ASSERT(count < color_program_expr_max_steps);
        steps[count++] = step;
    }
};

inline void
emit_get(color_program_expr& e, alia_color_channel channel)
{
    e.push_step(alia_color_step_get(channel));
}

inline void
emit_set(color_program_expr& e, alia_color_channel channel)
{
    e.push_step(alia_color_step_set(channel));
}

inline void
emit_immediate(color_program_expr& e, float value)
{
    e.push_step(alia_color_step_immediate(value));
}

inline void
emit_load_color(color_program_expr& e, alia_style_color_slot color_index)
{
    e.push_step(alia_color_step_load_color(color_index));
}

inline void
emit_load_var(color_program_expr& e, uint8_t float_index)
{
    e.push_step(alia_color_step_load_var(float_index));
}

// Cap TOS step to half of `|fg_l - bg_l|` for soft overlays.
// Float stack must be `[fg_l, bg_l, step]`; color TOS is the foreground.
// Leaves `[fg_l, bg_l, min(step, 0.5 * |fg_l - bg_l|)]`.
inline void
emit_cap_overlay_step(color_program_expr& e)
{
    emit_get(e, ALIA_COLOR_CHANNEL_L);
    emit_load_color(e, surface);
    emit_get(e, ALIA_COLOR_CHANNEL_L);
    e.push_step(alia_color_step_drop_color());
    e.push_step(alia_color_step_sub());
    e.push_step(alia_color_step_abs());
    emit_immediate(e, 0.5f);
    e.push_step(alia_color_step_mul());
    e.push_step(alia_color_step_min());
}

// Add `delta` onto TOS channel (GET, IMMEDIATE, ADD, SET).
inline void
emit_add_channel(color_program_expr& e, alia_color_channel channel, float delta)
{
    emit_get(e, channel);
    emit_immediate(e, delta);
    e.push_step(alia_color_step_add());
    emit_set(e, channel);
}

// Scale TOS channel by `factor` (GET, IMMEDIATE, MUL, SET).
inline void
emit_scale_channel(
    color_program_expr& e, alia_color_channel channel, float factor)
{
    emit_get(e, channel);
    emit_immediate(e, factor);
    e.push_step(alia_color_step_mul());
    emit_set(e, channel);
}

// Set TOS channel from an immediate.
inline void
emit_set_channel_immediate(
    color_program_expr& e, alia_color_channel channel, float value)
{
    emit_immediate(e, value);
    emit_set(e, channel);
}

// Set TOS channel from a palette float.
inline void
emit_set_channel_float(
    color_program_expr& e, alia_color_channel channel, uint8_t float_index)
{
    emit_load_var(e, float_index);
    emit_set(e, channel);
}

// Scale TOS channel by a palette float.
inline void
emit_scale_channel_float(
    color_program_expr& e, alia_color_channel channel, uint8_t float_index)
{
    emit_get(e, channel);
    emit_load_var(e, float_index);
    e.push_step(alia_color_step_mul());
    emit_set(e, channel);
}

// Copy channel from `ref` onto TOS (LOAD, GET, DROP, SET).
inline void
emit_match_channel(
    color_program_expr& e, alia_style_color_slot ref, alia_color_channel channel)
{
    emit_load_color(e, ref);
    emit_get(e, channel);
    e.push_step(alia_color_step_drop_color());
    emit_set(e, channel);
}

// Lerp TOS channel toward `ref` by `t`.
inline void
emit_lerp_channel(
    color_program_expr& e,
    alia_style_color_slot ref,
    alia_color_channel channel,
    float t)
{
    emit_get(e, channel);
    emit_load_color(e, ref);
    emit_get(e, channel);
    e.push_step(alia_color_step_drop_color());
    emit_immediate(e, t);
    e.push_step(alia_color_step_lerp());
    emit_set(e, channel);
}

// Lerp TOS channel toward `ref` by a style var.
inline void
emit_lerp_channel_var(
    color_program_expr& e,
    alia_style_color_slot ref,
    alia_color_channel channel,
    uint8_t var_index)
{
    emit_get(e, channel);
    emit_load_color(e, ref);
    emit_get(e, channel);
    e.push_step(alia_color_step_drop_color());
    emit_load_var(e, var_index);
    e.push_step(alia_color_step_lerp());
    emit_set(e, channel);
}

// Emit `t * (to.channel - from.channel)` then ADD onto TOS channel.
inline void
emit_channel_delta(
    color_program_expr& e,
    alia_style_color_slot from,
    alia_style_color_slot to,
    alia_color_channel channel,
    float t)
{
    emit_get(e, channel);
    emit_immediate(e, t);
    emit_load_color(e, to);
    emit_get(e, channel);
    e.push_step(alia_color_step_drop_color());
    emit_load_color(e, from);
    emit_get(e, channel);
    e.push_step(alia_color_step_drop_color());
    e.push_step(alia_color_step_sub());
    e.push_step(alia_color_step_mul());
    e.push_step(alia_color_step_add());
    emit_set(e, channel);
}

inline void
emit_channel_delta_float(
    color_program_expr& e,
    alia_style_color_slot from,
    alia_style_color_slot to,
    alia_color_channel channel,
    uint8_t float_index)
{
    emit_get(e, channel);
    emit_load_var(e, float_index);
    emit_load_color(e, to);
    emit_get(e, channel);
    e.push_step(alia_color_step_drop_color());
    emit_load_color(e, from);
    emit_get(e, channel);
    e.push_step(alia_color_step_drop_color());
    e.push_step(alia_color_step_sub());
    e.push_step(alia_color_step_mul());
    e.push_step(alia_color_step_add());
    emit_set(e, channel);
}

// Return an expression that loads palette `ref` (alpha 1).
inline color_program_expr
sample(alia_style_color_slot ref)
{
    color_program_expr e;
    emit_load_color(e, ref);
    return e;
}

// Sample TEXT, then ensure contrast against a pushed solid (prefer, auto).
inline color_program_expr
contrasting_ink(alia_style_color_slot solid_ref, float contrast_ratio = 4.5f)
{
    color_program_expr e;
    emit_load_color(e, text);
    emit_load_color(e, solid_ref);
    emit_immediate(e, contrast_ratio);
    e.push_step(alia_color_step_contrast());
    return e;
}

// Sample `ink_role`, then ensure contrast against a pushed solid.
inline color_program_expr
contrasting_ink(
    alia_style_color_slot solid_ref,
    float contrast_ratio,
    alia_style_color_slot ink_role,
    alia_color_l_direction prefer,
    alia_color_contrast_constraint constraint = ALIA_COLOR_CONTRAST_PREFER)
{
    color_program_expr e;
    emit_load_color(e, ink_role);
    emit_load_color(e, solid_ref);
    emit_immediate(e, contrast_ratio);
    e.push_step(alia_color_step_contrast_ex(prefer, constraint));
    return e;
}

// Return a view of an existing program span as an expression.
inline color_program_expr
view(alia_color_program_step const* steps, alia_color_program_ref ref)
{
    color_program_expr e;
    for (uint8_t i = 0; i < ref.count; ++i)
        e.push_step(steps[ref.offset + i]);
    return e;
}

// Append `tail` steps onto `head`.
inline color_program_expr
concat(color_program_expr head, color_program_expr const& tail)
{
    for (uint8_t i = 0; i < tail.count; ++i)
        head.push_step(tail.steps[i]);
    return head;
}

struct dup_op
{
};

inline dup_op
dup()
{
    return {};
}

struct drop_op
{
};

inline drop_op
drop()
{
    return {};
}

struct swap_op
{
};

inline swap_op
swap()
{
    return {};
}

// Alias for dup used by older call sites that saved the working color.
inline dup_op
push()
{
    return {};
}

struct pop_op
{
};

inline pop_op
pop()
{
    return {};
}

struct darken_op
{
    float amount;
};

inline darken_op
darken(float amount)
{
    return {amount};
}

struct lighten_op
{
    float amount;
};

inline lighten_op
lighten(float amount)
{
    return {amount};
}

struct add_lightness_delta_op
{
    alia_style_color_slot from;
    alia_style_color_slot to;
    float t;
};

inline add_lightness_delta_op
add_lightness_delta(alia_style_color_slot from, alia_style_color_slot to, float t)
{
    return {from, to, t};
}

struct add_chroma_delta_op
{
    alia_style_color_slot from;
    alia_style_color_slot to;
    float t;
};

inline add_chroma_delta_op
add_chroma_delta(alia_style_color_slot from, alia_style_color_slot to, float t)
{
    return {from, to, t};
}

struct add_hue_delta_op
{
    alia_style_color_slot from;
    alia_style_color_slot to;
    float t;
};

inline add_hue_delta_op
add_hue_delta(alia_style_color_slot from, alia_style_color_slot to, float t)
{
    return {from, to, t};
}

// Strengthen relative to the theme by adding `t * (TEXT.l - SURFACE.l)`.
struct strengthen_op
{
    float t;
};

inline strengthen_op
strengthen(float t)
{
    return {t};
}

struct strengthen_var_op
{
    uint8_t var_index;
};

inline strengthen_var_op
strengthen_var(uint8_t var_index)
{
    return {var_index};
}

struct weaken_op
{
    float t;
};

inline weaken_op
weaken(float t)
{
    return {t};
}

struct add_l_op
{
    float delta;
};

inline add_l_op
add_lightness(float delta)
{
    return {delta};
}

struct scale_l_op
{
    float factor;
};

inline scale_l_op
scale_lightness(float factor)
{
    return {factor};
}

struct scale_c_op
{
    float factor;
};

inline scale_c_op
scale_chroma(float factor)
{
    return {factor};
}

struct set_l_op
{
    float l;
};

inline set_l_op
set_lightness(float l)
{
    return {l};
}

struct set_c_op
{
    float c;
};

inline set_c_op
set_chroma(float c)
{
    return {c};
}

struct set_h_op
{
    float h_degrees;
};

inline set_h_op
set_hue(float h_degrees)
{
    return {h_degrees};
}

struct shift_h_op
{
    float delta_degrees;
};

inline shift_h_op
shift_hue(float delta_degrees)
{
    return {delta_degrees};
}

struct harmonize_h_op
{
    alia_style_color_slot ref;
    float step_degrees;
};

inline harmonize_h_op
harmonize_hue(
    alia_style_color_slot ref,
    float step_degrees = ALIA_COLOR_PROGRAM_HARMONIZE_STEP_DEG)
{
    return {ref, step_degrees};
}

struct match_l_op
{
    alia_style_color_slot ref;
};

inline match_l_op
match_lightness(alia_style_color_slot ref)
{
    return {ref};
}

struct match_c_op
{
    alia_style_color_slot ref;
};

inline match_c_op
match_chroma(alia_style_color_slot ref)
{
    return {ref};
}

struct match_h_op
{
    alia_style_color_slot ref;
};

inline match_h_op
match_hue(alia_style_color_slot ref)
{
    return {ref};
}

struct lerp_l_op
{
    alia_style_color_slot ref;
    float t;
};

inline lerp_l_op
lerp_lightness(alia_style_color_slot ref, float t)
{
    return {ref, t};
}

struct lerp_c_op
{
    alia_style_color_slot ref;
    float t;
};

inline lerp_c_op
lerp_chroma(alia_style_color_slot ref, float t)
{
    return {ref, t};
}

struct lerp_h_op
{
    alia_style_color_slot ref;
    float t;
};

inline lerp_h_op
lerp_hue(alia_style_color_slot ref, float t)
{
    return {ref, t};
}

struct fade_op
{
    alia_style_color_slot ref;
    float t_l;
    float t_c;
};

// Lerp L and C toward `ref` and match its H (former FADE toward surface).
inline fade_op
fade(alia_style_color_slot ref, float t_l, float t_c)
{
    return {ref, t_l, t_c};
}

struct fade_var_op
{
    alia_style_color_slot ref;
    uint8_t t_l_var;
    float t_c;
};

// Lerp L from a style var; C still takes an immediate blend factor.
inline fade_var_op
fade_var(alia_style_color_slot ref, uint8_t t_l_var, float t_c)
{
    return {ref, t_l_var, t_c};
}

struct blend_op
{
    alia_style_color_slot ref;
    float t;
};

inline blend_op
blend(alia_style_color_slot ref, float t)
{
    return {ref, t};
}

struct over_op
{
    alia_style_color_slot ref;
};

inline over_op
over(alia_style_color_slot ref)
{
    return {ref};
}

struct ensure_contrast_op
{
    alia_style_color_slot against;
    float ratio;
    alia_color_l_direction direction;
    alia_color_contrast_constraint constraint;
    alia_color_contrast_against against_mode;
};

inline ensure_contrast_op
ensure_contrast(
    alia_style_color_slot against,
    float ratio,
    alia_color_l_direction direction = ALIA_COLOR_L_AUTO,
    alia_color_contrast_constraint constraint = ALIA_COLOR_CONTRAST_PREFER)
{
    return {
        against,
        ratio,
        direction,
        constraint,
        ALIA_COLOR_CONTRAST_AGAINST_STACK};
}

struct ensure_contrast_var_op
{
    alia_style_color_slot against;
    uint8_t ratio_var;
    float scale;
    alia_color_l_direction direction;
    alia_color_contrast_constraint constraint;
};

// Contrast ratio is `vars[ratio_var] * scale` at resolve time.
inline ensure_contrast_var_op
ensure_contrast_var(
    alia_style_color_slot against,
    uint8_t ratio_var,
    float scale = 1.f,
    alia_color_l_direction direction = ALIA_COLOR_L_AUTO,
    alia_color_contrast_constraint constraint = ALIA_COLOR_CONTRAST_PREFER)
{
    return {against, ratio_var, scale, direction, constraint};
}

inline ensure_contrast_op
ensure_contrast_against_white(
    float ratio,
    alia_color_l_direction direction,
    alia_color_contrast_constraint constraint)
{
    return {
        0,
        ratio,
        direction,
        constraint,
        ALIA_COLOR_CONTRAST_AGAINST_WHITE};
}

inline ensure_contrast_op
ensure_contrast_against_black(
    float ratio,
    alia_color_l_direction direction,
    alia_color_contrast_constraint constraint)
{
    return {
        0,
        ratio,
        direction,
        constraint,
        ALIA_COLOR_CONTRAST_AGAINST_BLACK};
}

struct set_alpha_op
{
    float alpha;
};

inline set_alpha_op
set_alpha(float alpha)
{
    return {alpha};
}

struct scale_alpha_op
{
    float factor;
};

inline scale_alpha_op
scale_alpha(float factor)
{
    return {factor};
}

struct set_highlight_alpha_op
{
    float l_step;
};

inline set_highlight_alpha_op
set_highlight_alpha(float l_step)
{
    return {l_step};
}

// Sample `role` and apply highlight coverage for L-step `highlight_l`.
inline color_program_expr
highlight(alia_style_color_slot role, float highlight_l)
{
    color_program_expr e;
    emit_load_color(e, role);
    emit_get(e, ALIA_COLOR_CHANNEL_L);
    emit_load_color(e, surface);
    emit_get(e, ALIA_COLOR_CHANNEL_L);
    e.push_step(alia_color_step_drop_color());
    emit_immediate(e, highlight_l);
    emit_cap_overlay_step(e);
    e.push_step(alia_color_step_overlay());
    emit_set(e, ALIA_COLOR_CHANNEL_A);
    return e;
}

// Sample `role` and apply highlight coverage from a style var L-step.
inline color_program_expr
highlight_var(alia_style_color_slot role, uint8_t highlight_l_var)
{
    color_program_expr e;
    emit_load_color(e, role);
    emit_get(e, ALIA_COLOR_CHANNEL_L);
    emit_load_color(e, surface);
    emit_get(e, ALIA_COLOR_CHANNEL_L);
    e.push_step(alia_color_step_drop_color());
    emit_load_var(e, highlight_l_var);
    emit_cap_overlay_step(e);
    e.push_step(alia_color_step_overlay());
    emit_set(e, ALIA_COLOR_CHANNEL_A);
    return e;
}

struct add_lightness_delta_float_op
{
    alia_style_color_slot from;
    alia_style_color_slot to;
    uint8_t float_index;
};

inline add_lightness_delta_float_op
add_lightness_delta_float(
    alia_style_color_slot from, alia_style_color_slot to, uint8_t float_index)
{
    return {from, to, float_index};
}

struct scale_chroma_float_op
{
    uint8_t float_index;
};

inline scale_chroma_float_op
scale_chroma_float(uint8_t float_index)
{
    return {float_index};
}

struct set_lightness_float_op
{
    uint8_t float_index;
};

inline set_lightness_float_op
set_lightness_float(uint8_t float_index)
{
    return {float_index};
}

inline color_program_expr
operator|(color_program_expr e, dup_op)
{
    e.push_step(alia_color_step_dup_color());
    return e;
}

inline color_program_expr
operator|(color_program_expr e, drop_op)
{
    e.push_step(alia_color_step_drop_color());
    return e;
}

inline color_program_expr
operator|(color_program_expr e, swap_op)
{
    e.push_step(alia_color_step_swap_color());
    return e;
}

inline color_program_expr
operator|(color_program_expr e, pop_op)
{
    e.push_step(alia_color_step_drop_color());
    return e;
}

inline color_program_expr
operator|(color_program_expr e, darken_op op)
{
    emit_add_channel(e, ALIA_COLOR_CHANNEL_L, -op.amount);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, lighten_op op)
{
    emit_add_channel(e, ALIA_COLOR_CHANNEL_L, op.amount);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, add_lightness_delta_op op)
{
    emit_channel_delta(
        e, op.from, op.to, ALIA_COLOR_CHANNEL_L, op.t);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, add_chroma_delta_op op)
{
    emit_channel_delta(
        e, op.from, op.to, ALIA_COLOR_CHANNEL_C, op.t);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, add_hue_delta_op op)
{
    // Shortest arc is not preserved with raw subtract - acceptable for now.
    emit_channel_delta(
        e, op.from, op.to, ALIA_COLOR_CHANNEL_H, op.t);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, strengthen_op op)
{
    emit_channel_delta(e, surface, text, ALIA_COLOR_CHANNEL_L, op.t);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, strengthen_var_op op)
{
    emit_channel_delta_float(
        e, surface, text, ALIA_COLOR_CHANNEL_L, op.var_index);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, weaken_op op)
{
    emit_channel_delta(e, surface, text, ALIA_COLOR_CHANNEL_L, -op.t);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, add_l_op op)
{
    emit_add_channel(e, ALIA_COLOR_CHANNEL_L, op.delta);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, scale_l_op op)
{
    emit_scale_channel(e, ALIA_COLOR_CHANNEL_L, op.factor);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, scale_c_op op)
{
    emit_scale_channel(e, ALIA_COLOR_CHANNEL_C, op.factor);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, set_l_op op)
{
    emit_set_channel_immediate(e, ALIA_COLOR_CHANNEL_L, op.l);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, set_c_op op)
{
    emit_set_channel_immediate(e, ALIA_COLOR_CHANNEL_C, op.c);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, set_h_op op)
{
    emit_set_channel_immediate(
        e,
        ALIA_COLOR_CHANNEL_H,
        alia_color_program_deg_to_rad(op.h_degrees));
    return e;
}

inline color_program_expr
operator|(color_program_expr e, shift_h_op op)
{
    emit_add_channel(
        e,
        ALIA_COLOR_CHANNEL_H,
        alia_color_program_deg_to_rad(op.delta_degrees));
    return e;
}

inline color_program_expr
operator|(color_program_expr e, harmonize_h_op op)
{
    emit_get(e, ALIA_COLOR_CHANNEL_H);
    emit_load_color(e, op.ref);
    emit_get(e, ALIA_COLOR_CHANNEL_H);
    e.push_step(alia_color_step_drop_color());
    emit_immediate(e, alia_color_program_deg_to_rad(op.step_degrees));
    e.push_step(alia_color_step_snap());
    emit_set(e, ALIA_COLOR_CHANNEL_H);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, match_l_op op)
{
    emit_match_channel(e, op.ref, ALIA_COLOR_CHANNEL_L);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, match_c_op op)
{
    emit_match_channel(e, op.ref, ALIA_COLOR_CHANNEL_C);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, match_h_op op)
{
    emit_match_channel(e, op.ref, ALIA_COLOR_CHANNEL_H);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, lerp_l_op op)
{
    emit_lerp_channel(e, op.ref, ALIA_COLOR_CHANNEL_L, op.t);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, lerp_c_op op)
{
    emit_lerp_channel(e, op.ref, ALIA_COLOR_CHANNEL_C, op.t);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, lerp_h_op op)
{
    emit_lerp_channel(e, op.ref, ALIA_COLOR_CHANNEL_H, op.t);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, fade_op op)
{
    emit_lerp_channel(e, op.ref, ALIA_COLOR_CHANNEL_L, op.t_l);
    emit_lerp_channel(e, op.ref, ALIA_COLOR_CHANNEL_C, op.t_c);
    emit_match_channel(e, op.ref, ALIA_COLOR_CHANNEL_H);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, fade_var_op op)
{
    emit_lerp_channel_var(e, op.ref, ALIA_COLOR_CHANNEL_L, op.t_l_var);
    emit_lerp_channel(e, op.ref, ALIA_COLOR_CHANNEL_C, op.t_c);
    emit_match_channel(e, op.ref, ALIA_COLOR_CHANNEL_H);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, blend_op op)
{
    emit_load_color(e, op.ref);
    emit_immediate(e, op.t);
    e.push_step(alia_color_step_blend());
    return e;
}

inline color_program_expr
operator|(color_program_expr e, over_op op)
{
    // Current TOS is source. Need [under, source] then OVER.
    emit_load_color(e, op.ref);
    e.push_step(alia_color_step_swap_color());
    e.push_step(alia_color_step_over());
    return e;
}

inline color_program_expr
operator|(color_program_expr e, ensure_contrast_op op)
{
    if (op.against_mode == ALIA_COLOR_CONTRAST_AGAINST_STACK)
        emit_load_color(e, op.against);
    emit_immediate(e, op.ratio);
    if (op.against_mode == ALIA_COLOR_CONTRAST_AGAINST_WHITE)
    {
        e.push_step(alia_color_step_contrast_against_white(
            op.direction, op.constraint));
    }
    else if (op.against_mode == ALIA_COLOR_CONTRAST_AGAINST_BLACK)
    {
        e.push_step(alia_color_step_contrast_against_black(
            op.direction, op.constraint));
    }
    else
    {
        e.push_step(alia_color_step_contrast_ex(op.direction, op.constraint));
    }
    return e;
}

inline color_program_expr
operator|(color_program_expr e, ensure_contrast_var_op op)
{
    emit_load_color(e, op.against);
    emit_load_var(e, op.ratio_var);
    emit_immediate(e, op.scale);
    e.push_step(alia_color_step_mul());
    e.push_step(alia_color_step_contrast_ex(op.direction, op.constraint));
    return e;
}

inline color_program_expr
operator|(color_program_expr e, set_alpha_op op)
{
    emit_set_channel_immediate(e, ALIA_COLOR_CHANNEL_A, op.alpha);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, scale_alpha_op op)
{
    emit_scale_channel(e, ALIA_COLOR_CHANNEL_A, op.factor);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, set_highlight_alpha_op op)
{
    emit_get(e, ALIA_COLOR_CHANNEL_L);
    emit_load_color(e, surface);
    emit_get(e, ALIA_COLOR_CHANNEL_L);
    e.push_step(alia_color_step_drop_color());
    emit_immediate(e, op.l_step);
    emit_cap_overlay_step(e);
    e.push_step(alia_color_step_overlay());
    emit_set(e, ALIA_COLOR_CHANNEL_A);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, add_lightness_delta_float_op op)
{
    emit_channel_delta_float(
        e, op.from, op.to, ALIA_COLOR_CHANNEL_L, op.float_index);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, scale_chroma_float_op op)
{
    emit_scale_channel_float(e, ALIA_COLOR_CHANNEL_C, op.float_index);
    return e;
}

inline color_program_expr
operator|(color_program_expr e, set_lightness_float_op op)
{
    emit_set_channel_float(e, ALIA_COLOR_CHANNEL_L, op.float_index);
    return e;
}

// placement of a role solid (mirrors `alia_color_tone_t`)
enum class tone : uint32_t
{
    match = ALIA_COLOR_TONE_MATCH,
    dark = ALIA_COLOR_TONE_DARK,
    light = ALIA_COLOR_TONE_LIGHT,
    inverse = ALIA_COLOR_TONE_INVERSE,
};

// Build a role solid fill from palette tone floats (SURFACE-to-TEXT step or
// absolute L).
inline color_program_expr
role_solid_fill(alia_style_color_slot role, tone t)
{
    switch (t)
    {
        case tone::dark:
            return sample(role) | set_lightness_float(ALIA_STYLE_VAR_DARK_L)
                 | scale_chroma_float(ALIA_STYLE_VAR_ABSOLUTE_CHROMA);
        case tone::light:
            return sample(role) | set_lightness_float(ALIA_STYLE_VAR_LIGHT_L)
                 | scale_chroma_float(ALIA_STYLE_VAR_ABSOLUTE_CHROMA);
        case tone::inverse:
            return sample(role) | match_lightness(surface)
                 | add_lightness_delta_float(
                       surface, text, ALIA_STYLE_VAR_INVERSE_STEP)
                 | scale_chroma_float(ALIA_STYLE_VAR_INVERSE_CHROMA);
        case tone::match:
        default:
            return sample(role) | match_lightness(surface)
                 | add_lightness_delta_float(
                       surface, text, ALIA_STYLE_VAR_MATCH_STEP)
                 | scale_chroma_float(ALIA_STYLE_VAR_MATCH_CHROMA);
    }
}

// Build black/white on-color against `role_solid_fill` (not theme text).
// Contrast uses `ALIA_STYLE_VAR_CONTRAST_TARGET * contrast_scale`.
inline color_program_expr
role_solid_on_color(
    alia_style_color_slot role, tone t, float contrast_scale = 1.5f)
{
    color_program_expr e = role_solid_fill(role, t);
    e.push_step(alia_color_step_dup_color());
    emit_set_channel_immediate(e, ALIA_COLOR_CHANNEL_C, 0.f);
    emit_set_channel_immediate(e, ALIA_COLOR_CHANNEL_L, 1.f);
    e.push_step(alia_color_step_swap_color());
    emit_load_var(e, ALIA_STYLE_VAR_CONTRAST_TARGET);
    emit_immediate(e, contrast_scale);
    e.push_step(alia_color_step_mul());
    e.push_step(alia_color_step_contrast_ex(
        ALIA_COLOR_L_AUTO, ALIA_COLOR_CONTRAST_REQUIRE));
    return e;
}

// Emit `expr` into `emitter` and return the relative ref.
inline alia_color_program_ref
emit(alia_color_program_emitter* emitter, color_program_expr const& expr)
{
    uint16_t const start = alia_color_program_emitter_count(emitter);
    for (uint8_t i = 0; i < expr.count; ++i)
    {
        if (alia_color_program_emit_step(emitter, expr.steps[i]) != 0)
            return alia_color_program_ref_none();
    }
    return alia_color_program_end_ref(emitter, start);
}

// Copy `ref` from `src` into `emitter` (no suffix).
inline alia_color_program_ref
emit_copy_ref(
    alia_color_program_emitter* emitter,
    alia_color_program_step const* src,
    alia_color_program_ref ref)
{
    return alia_color_program_emit_copy(emitter, src, ref);
}

// Copy `ref` from `src`, append `suffix`, and emit the combined span.
inline alia_color_program_ref
emit_append(
    alia_color_program_emitter* emitter,
    alia_color_program_step const* src,
    alia_color_program_ref ref,
    color_program_expr const& suffix)
{
    if (ref.count == 0 && suffix.count == 0)
        return alia_color_program_ref_none();
    return emit(emitter, concat(view(src, ref), suffix));
}

// rest program plus interaction continuations, as expressions
struct responsive_color_expr
{
    color_program_expr rest{};
    color_program_expr hovered{};
    color_program_expr active{};
    color_program_expr disabled{};
};

// Emit `expr` as rest + continuations into `emitter`.
inline alia_responsive_color_program
emit_responsive(
    alia_color_program_emitter* emitter, responsive_color_expr const& expr)
{
    alia_responsive_color_program out;
    out.rest = emit(emitter, expr.rest);
    out.hovered = emit(emitter, expr.hovered);
    out.active = emit(emitter, expr.active);
    out.disabled = emit(emitter, expr.disabled);
    return out;
}

// Re-emit `in` from `src`, appending `rest_suffix` only onto non-empty rest.
// Continuations are copied unchanged so seed-based hover/active keep working.
inline alia_responsive_color_program
emit_responsive_append_rest(
    alia_color_program_emitter* emitter,
    alia_color_program_step const* src,
    alia_responsive_color_program const& in,
    color_program_expr const& rest_suffix)
{
    alia_responsive_color_program out;
    out.rest = emit_append(emitter, src, in.rest, rest_suffix);
    out.hovered = emit_copy_ref(emitter, src, in.hovered);
    out.active = emit_copy_ref(emitter, src, in.active);
    out.disabled = emit_copy_ref(emitter, src, in.disabled);
    return out;
}

} // namespace color
} // namespace alia
