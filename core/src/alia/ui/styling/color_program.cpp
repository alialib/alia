#include <alia/abi/ui/styling/color_program.h>

#include <alia/abi/base/allocator.h>
#include <alia/abi/base/color.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/input/elements.h>
#include <alia/abi/ui/styling/environment.h>

#include <cmath>
#include <cstdio>
#include <cstring>

static_assert(
    (sizeof(alia_color_program_step) & (ALIA_MIN_ALIGN - 1)) == 0,
    "color program steps must be arena-aligned");

namespace {

float
clamp01(float x)
{
    return x < 0.f ? 0.f : (x > 1.f ? 1.f : x);
}

// Snap `hue` to the lattice of `step` around `accent` (all radians).
float
harmonize_hue(float hue, float accent, float step)
{
    if (step <= 0.f)
        return accent;
    float const delta = hue - accent;
    float const remainder = delta - step * std::round(delta / step);
    return hue - remainder;
}

uint8_t
alpha_u8(float a)
{
    float const c = clamp01(a);
    return (uint8_t) (c * 255.f + 0.5f);
}

struct stacked_color
{
    alia_oklch c{};
    float a = 1.f;
};

struct color_stack
{
    stacked_color slots[ALIA_COLOR_PROGRAM_STACK_DEPTH]{};
    uint8_t depth = 0;
};

struct float_stack
{
    float slots[ALIA_COLOR_FLOAT_STACK_DEPTH]{};
    uint8_t depth = 0;
};

alia_oklch
color_oklch(alia_style_environment const* env, uint8_t slot)
{
    return env->palette.colors[slot < ALIA_STYLE_COLOR_COUNT ? slot : 0];
}

stacked_color
slot_color(alia_style_environment const* env, uint8_t slot)
{
    return {color_oklch(env, slot), 1.f};
}

void
push_color(color_stack* stack, stacked_color value)
{
    ALIA_ASSERT(stack->depth < ALIA_COLOR_PROGRAM_STACK_DEPTH);
    stack->slots[stack->depth++] = value;
}

stacked_color
pop_color(color_stack* stack)
{
    ALIA_ASSERT(stack->depth > 0);
    return stack->slots[--stack->depth];
}

stacked_color*
tos_color(color_stack* stack)
{
    ALIA_ASSERT(stack->depth > 0);
    return &stack->slots[stack->depth - 1];
}

void
push_float(float_stack* stack, float value)
{
    ALIA_ASSERT(stack->depth < ALIA_COLOR_FLOAT_STACK_DEPTH);
    stack->slots[stack->depth++] = value;
}

float
pop_float(float_stack* stack)
{
    ALIA_ASSERT(stack->depth > 0);
    return stack->slots[--stack->depth];
}

alia_srgba8
blend_over(alia_srgba8 dst_s, alia_srgba8 src_s)
{
    alia_rgba const dst = alia_rgba_from_srgba8(dst_s);
    alia_rgba const src = alia_rgba_from_srgba8(src_s);
    float const out_a = src.a + dst.a * (1.f - src.a);
    if (out_a <= 0.f)
        return alia_srgba8_make(0, 0, 0, 0);
    float const inv = 1.f / out_a;
    float const dst_weight = dst.a * (1.f - src.a);
    return alia_srgba8_from_rgba(
        alia_rgba{
            (src.r * src.a + dst.r * dst_weight) * inv,
            (src.g * src.a + dst.g * dst_weight) * inv,
            (src.b * src.a + dst.b * dst_weight) * inv,
            out_a});
}

alia_srgba8
emit_srgba(stacked_color const& value)
{
    alia_srgb8 const rgb = alia_srgb8_from_unclamped_oklch(value.c);
    return alia_srgba8_make(rgb.r, rgb.g, rgb.b, alpha_u8(value.a));
}

float
contrast_ratio_oklch(alia_oklch a, alia_oklch b)
{
    float const la
        = alia_relative_luminance_srgb8(alia_srgb8_from_unclamped_oklch(a));
    float const lb
        = alia_relative_luminance_srgb8(alia_srgb8_from_unclamped_oklch(b));
    return alia_relative_luminance_ratio(la, lb);
}

// Resolve whether the preferred search moves lightness upward.
bool
prefer_upward(
    alia_oklch working,
    alia_oklch against,
    alia_style_environment const* env,
    alia_color_l_direction direction)
{
    switch (direction)
    {
        case ALIA_COLOR_L_LIGHT:
            return true;
        case ALIA_COLOR_L_DARK:
            return false;
        case ALIA_COLOR_L_STRONG: {
            // Use the theme axis in the same direction as strengthen (toward
            // TEXT from SURFACE), independent of the working color.
            (void) working;
            (void) against;
            return color_oklch(env, ALIA_STYLE_COLOR_TEXT).l
                >= color_oklch(env, ALIA_STYLE_COLOR_SURFACE).l;
        }
        case ALIA_COLOR_L_WEAK: {
            (void) working;
            (void) against;
            return color_oklch(env, ALIA_STYLE_COLOR_SURFACE).l
                >= color_oklch(env, ALIA_STYLE_COLOR_TEXT).l;
        }
        case ALIA_COLOR_L_AUTO:
        default: {
            // Prefer the ink pole (near-white vs near-black) with higher
            // contrast against the fill. Relative luminance captures chroma
            // effects that raw OKLCH L misses (e.g. ochre vs navy at similar
            // L).
            (void) working;
            alia_oklch const white = alia_oklch_make(1.f, 0.f, 0.f);
            alia_oklch const black = alia_oklch_make(0.f, 0.f, 0.f);
            return contrast_ratio_oklch(white, against)
                >= contrast_ratio_oklch(black, against);
        }
    }
}

// Adjust working.L until contrast vs `against` meets `target`.
void
ensure_contrast(
    alia_oklch* working,
    alia_oklch against,
    float target,
    alia_style_environment const* env,
    alia_color_l_direction direction,
    alia_color_contrast_constraint constraint)
{
    bool const prefer_up = prefer_upward(*working, against, env, direction);

    // For achromatic AUTO ink, snap to the winning black/white pole used for
    // solid on-color. A gradual L search would stop at a gray that only barely
    // meets `target`.
    if (direction == ALIA_COLOR_L_AUTO && working->c < 1e-3f)
    {
        working->l = prefer_up ? 1.f : 0.f;
        working->c = 0.f;
        return;
    }

    if (contrast_ratio_oklch(*working, against) >= target)
        return;

    float best_l = working->l;
    float best_ratio = contrast_ratio_oklch(*working, against);

    auto consider = [&](bool upward) -> bool {
        alia_oklch candidate = *working;
        float lo = upward ? working->l : 0.f;
        float hi = upward ? 1.f : working->l;
        for (int i = 0; i < 24; ++i)
        {
            float const mid = 0.5f * (lo + hi);
            candidate.l = mid;
            float const ratio = contrast_ratio_oklch(candidate, against);
            if (ratio >= target)
            {
                working->l = mid;
                return true;
            }
            if (upward)
                lo = mid;
            else
                hi = mid;
            if (ratio > best_ratio)
            {
                best_ratio = ratio;
                best_l = candidate.l;
            }
        }
        return false;
    };

    if (constraint == ALIA_COLOR_CONTRAST_REQUIRE)
    {
        if (!consider(prefer_up))
            working->l = best_l;
        return;
    }

    if (consider(prefer_up) || consider(!prefer_up))
        return;
    working->l = best_l;
}

float
channel_get(stacked_color const& value, alia_color_channel channel)
{
    switch (channel)
    {
        case ALIA_COLOR_CHANNEL_L:
            return value.c.l;
        case ALIA_COLOR_CHANNEL_C:
            return value.c.c;
        case ALIA_COLOR_CHANNEL_H:
            return value.c.h;
        case ALIA_COLOR_CHANNEL_A:
            return value.a;
        default:
            return 0.f;
    }
}

void
channel_set(stacked_color* value, alia_color_channel channel, float x)
{
    switch (channel)
    {
        case ALIA_COLOR_CHANNEL_L:
            value->c.l = clamp01(x);
            break;
        case ALIA_COLOR_CHANNEL_C:
            value->c.c = x;
            break;
        case ALIA_COLOR_CHANNEL_H:
            value->c.h = x;
            break;
        case ALIA_COLOR_CHANNEL_A:
            value->a = clamp01(x);
            break;
        default:
            break;
    }
}

void
eval_span(
    alia_style_environment const* env,
    alia_color_program_step const* steps,
    uint16_t offset,
    uint8_t count,
    color_stack* colors,
    float_stack* floats)
{
    for (uint8_t i = 0; i < count; ++i)
    {
        alia_color_program_step const* step = &steps[offset + i];
        switch (step->op)
        {
            case ALIA_COLOR_OP_LOAD_COLOR:
                push_color(colors, slot_color(env, step->u8));
                break;
            case ALIA_COLOR_OP_LOAD_VAR:
                push_float(floats, alia_style_var(env, step->u8));
                break;
            case ALIA_COLOR_OP_IMMEDIATE:
                push_float(floats, step->f);
                break;
            case ALIA_COLOR_OP_DUP_COLOR: {
                stacked_color const top = *tos_color(colors);
                push_color(colors, top);
                break;
            }
            case ALIA_COLOR_OP_DROP_COLOR:
                (void) pop_color(colors);
                break;
            case ALIA_COLOR_OP_SWAP_COLOR: {
                ALIA_ASSERT(colors->depth >= 2);
                stacked_color tmp = colors->slots[colors->depth - 1];
                colors->slots[colors->depth - 1]
                    = colors->slots[colors->depth - 2];
                colors->slots[colors->depth - 2] = tmp;
                break;
            }
            case ALIA_COLOR_OP_DUP_FLOAT: {
                ALIA_ASSERT(floats->depth > 0);
                push_float(floats, floats->slots[floats->depth - 1]);
                break;
            }
            case ALIA_COLOR_OP_DROP_FLOAT:
                (void) pop_float(floats);
                break;
            case ALIA_COLOR_OP_SWAP_FLOAT: {
                ALIA_ASSERT(floats->depth >= 2);
                float tmp = floats->slots[floats->depth - 1];
                floats->slots[floats->depth - 1]
                    = floats->slots[floats->depth - 2];
                floats->slots[floats->depth - 2] = tmp;
                break;
            }
            case ALIA_COLOR_OP_ADD: {
                float const a = pop_float(floats);
                float const b = pop_float(floats);
                push_float(floats, b + a);
                break;
            }
            case ALIA_COLOR_OP_SUB: {
                float const a = pop_float(floats);
                float const b = pop_float(floats);
                push_float(floats, b - a);
                break;
            }
            case ALIA_COLOR_OP_MUL: {
                float const a = pop_float(floats);
                float const b = pop_float(floats);
                push_float(floats, b * a);
                break;
            }
            case ALIA_COLOR_OP_ABS: {
                float const a = pop_float(floats);
                push_float(floats, a < 0.f ? -a : a);
                break;
            }
            case ALIA_COLOR_OP_MIN: {
                float const a = pop_float(floats);
                float const b = pop_float(floats);
                push_float(floats, b < a ? b : a);
                break;
            }
            case ALIA_COLOR_OP_LERP: {
                float const t = pop_float(floats);
                float const b = pop_float(floats);
                float const a = pop_float(floats);
                push_float(floats, a + (b - a) * t);
                break;
            }
            case ALIA_COLOR_OP_SNAP: {
                float const step_h = pop_float(floats);
                float const accent = pop_float(floats);
                float const hue = pop_float(floats);
                push_float(floats, harmonize_hue(hue, accent, step_h));
                break;
            }
            case ALIA_COLOR_OP_OVERLAY: {
                float const l_step = pop_float(floats);
                float const bg_l = pop_float(floats);
                float const fg_l = pop_float(floats);
                push_float(
                    floats, alia_oklch_overlay_alpha(bg_l, fg_l, l_step));
                break;
            }
            case ALIA_COLOR_OP_GET:
                push_float(
                    floats,
                    channel_get(
                        *tos_color(colors), (alia_color_channel) step->u8));
                break;
            case ALIA_COLOR_OP_SET:
                channel_set(
                    tos_color(colors),
                    (alia_color_channel) step->u8,
                    pop_float(floats));
                break;
            case ALIA_COLOR_OP_BLEND: {
                float const t = clamp01(pop_float(floats));
                stacked_color const target = pop_color(colors);
                stacked_color* tos = tos_color(colors);
                float const saved_a = tos->a;
                tos->c = alia_lerp_oklch(tos->c, target.c, t);
                tos->a = saved_a;
                break;
            }
            case ALIA_COLOR_OP_OVER: {
                stacked_color const src = pop_color(colors);
                stacked_color const dst = pop_color(colors);
                alia_srgba8 const out
                    = blend_over(emit_srgba(dst), emit_srgba(src));
                stacked_color result;
                result.c = alia_oklch_from_srgb8(
                    alia_srgb8_make(out.r, out.g, out.b));
                result.a = (float) out.a / 255.f;
                push_color(colors, result);
                break;
            }
            case ALIA_COLOR_OP_CONTRAST: {
                alia_color_l_direction const direction
                    = alia_color_contrast_direction(step->u8);
                alia_color_contrast_constraint const constraint
                    = alia_color_contrast_constraint_of(step->u8);
                alia_color_contrast_against const against_mode
                    = alia_color_contrast_against_of(step->u8);
                float target = pop_float(floats);
                if (target <= 0.f)
                    target = 4.5f;
                alia_oklch against;
                switch (against_mode)
                {
                    case ALIA_COLOR_CONTRAST_AGAINST_WHITE:
                        against = alia_oklch_make(1.f, 0.f, 0.f);
                        break;
                    case ALIA_COLOR_CONTRAST_AGAINST_BLACK:
                        against = alia_oklch_make(0.f, 0.f, 0.f);
                        break;
                    case ALIA_COLOR_CONTRAST_AGAINST_STACK:
                    default:
                        against = pop_color(colors).c;
                        break;
                }
                ensure_contrast(
                    &tos_color(colors)->c,
                    against,
                    target,
                    env,
                    direction,
                    constraint);
                break;
            }
            default:
                break;
        }
    }
}

stacked_color
eval_program(
    alia_style_environment const* env,
    alia_color_program_step const* steps,
    uint16_t offset,
    uint8_t count,
    stacked_color const* seed)
{
    color_stack colors{};
    float_stack floats{};
    if (seed)
        push_color(&colors, *seed);
    eval_span(env, steps, offset, count, &colors, &floats);
    if (colors.depth == 0)
        return slot_color(env, ALIA_STYLE_COLOR_SURFACE);
    return *tos_color(&colors);
}

char const* const op_descriptions[] = {
#define ALIA_COLOR_OP_DESC(NAME, desc) desc,
    ALIA_COLOR_OPS(ALIA_COLOR_OP_DESC)
#undef ALIA_COLOR_OP_DESC
};

char const* const op_names[] = {
#define ALIA_COLOR_OP_NAME(NAME, desc) #NAME,
    ALIA_COLOR_OPS(ALIA_COLOR_OP_NAME)
#undef ALIA_COLOR_OP_NAME
};

char const*
op_name(uint8_t op)
{
    if (op >= ALIA_COLOR_OP_COUNT)
        return "?";
    return op_names[op];
}

} // namespace

extern "C" {

char const*
alia_color_op_description(uint8_t op)
{
    if (op >= ALIA_COLOR_OP_COUNT)
        return nullptr;
    return op_descriptions[op];
}

alia_color_program_emitter
alia_color_program_emitter_begin(alia_bump_allocator* bump)
{
    ALIA_ASSERT(bump);
    alia_color_program_emitter emitter;
    emitter.bump = bump;
    emitter.base = nullptr;
    emitter.count = 0;
    return emitter;
}

int
alia_color_program_emit_step(
    alia_color_program_emitter* emitter, alia_color_program_step step)
{
    ALIA_ASSERT(emitter);
    ALIA_ASSERT(emitter->bump);
    if (emitter->count == UINT16_MAX)
        return -1;

    if (emitter->base != nullptr)
    {
        // Region must stay contiguous in the bump.
        ALIA_ASSERT(
            emitter->bump->offset
            == alia_arena_offset(emitter->bump, emitter->base)
                   + sizeof(alia_color_program_step) * emitter->count);
    }

    alia_offset const off
        = alia_arena_alloc(emitter->bump, sizeof(alia_color_program_step));
    auto* slot = static_cast<alia_color_program_step*>(
        alia_arena_ptr(emitter->bump, off));
    *slot = step;
    if (emitter->base == nullptr)
        emitter->base = slot;
    ++emitter->count;
    return 0;
}

alia_color_program_ref
alia_color_program_emit_copy(
    alia_color_program_emitter* emitter,
    alia_color_program_step const* src,
    alia_color_program_ref ref)
{
    ALIA_ASSERT(emitter);
    uint16_t const start = emitter->count;
    for (uint8_t i = 0; i < ref.count; ++i)
    {
        if (alia_color_program_emit_step(emitter, src[ref.offset + i]) != 0)
            return alia_color_program_ref_none();
    }
    return alia_color_program_end_ref(emitter, start);
}

alia_color_program_ref
alia_color_program_end_ref(
    alia_color_program_emitter const* emitter, uint16_t start)
{
    ALIA_ASSERT(emitter);
    ALIA_ASSERT(start <= emitter->count);
    alia_color_program_ref ref;
    ref.offset = start;
    uint16_t const n = (uint16_t) (emitter->count - start);
    ref.count = n > 255 ? (uint8_t) 255 : (uint8_t) n;
    return ref;
}

alia_color_program_step*
alia_color_program_materialize(
    alia_kernel_context* ctx,
    alia_color_program_emitter const* emitter,
    alia_general_allocator* allocator,
    uint32_t flags)
{
    ALIA_ASSERT(emitter);
    if (emitter->count == 0)
        return nullptr;

    size_t const bytes = sizeof(alia_color_program_step) * emitter->count;
    alia_color_program_step* dst = nullptr;

    if (flags & ALIA_COLOR_PROGRAM_MATERIALIZE_FOLD)
    {
        ALIA_ASSERT(ctx);
        // Size must be stable across visits. The substrate enforces layout.
        alia_substrate_usage_result const usage = alia_substrate_use_memory(
            ctx, bytes, alignof(alia_color_program_step));
        dst = static_cast<alia_color_program_step*>(usage.ptr);
        std::memcpy(dst, emitter->base, bytes);
    }
    else
    {
        ALIA_ASSERT(allocator);
        ALIA_ASSERT(allocator->alloc);
        dst = static_cast<alia_color_program_step*>(allocator->alloc(
            allocator->user_data, bytes, alignof(alia_color_program_step)));
        ALIA_ASSERT(dst);
        std::memcpy(dst, emitter->base, bytes);
    }
    return dst;
}

alia_srgba8
alia_color_program_resolve(
    alia_style_environment const* env,
    alia_color_program_step const* steps,
    alia_color_program_ref ref)
{
    ALIA_ASSERT(env);
    ALIA_ASSERT(steps || ref.count == 0);
    stacked_color const result
        = eval_program(env, steps, ref.offset, ref.count, nullptr);
    return emit_srgba(result);
}

alia_srgba8
alia_responsive_color_program_resolve(
    alia_style_environment const* env,
    alia_color_program_step const* steps,
    alia_responsive_color_program const* color,
    uint8_t status)
{
    ALIA_ASSERT(env);
    ALIA_ASSERT(color);
    ALIA_ASSERT(steps || color->rest.count == 0);
    stacked_color result = eval_program(
        env, steps, color->rest.offset, color->rest.count, nullptr);

    alia_color_program_ref cont = alia_color_program_ref_none();
    if (status & ALIA_INTERACTION_STATUS_DISABLED)
        cont = color->disabled;
    else if (status & ALIA_INTERACTION_STATUS_ACTIVE)
        cont = color->active;
    else if (status & ALIA_INTERACTION_STATUS_HOVERED)
        cont = color->hovered;

    if (cont.count != 0)
        result = eval_program(env, steps, cont.offset, cont.count, &result);

    return emit_srgba(result);
}

int
alia_color_program_snprint(
    char* buf,
    int buf_size,
    alia_color_program_step const* steps,
    alia_color_program_ref ref)
{
    if (!buf || buf_size <= 0)
        return 0;
    int written = 0;
    for (uint8_t i = 0; i < ref.count; ++i)
    {
        if (written >= buf_size - 1)
            break;
        alia_color_program_step const& s = steps[ref.offset + i];
        char const* name = op_name(s.op);
        char line[128];
        if (s.op == ALIA_COLOR_OP_LOAD_COLOR)
        {
            std::snprintf(
                line, sizeof(line), "%s color=%u\n", name, (unsigned) s.u8);
        }
        else if (s.op == ALIA_COLOR_OP_LOAD_VAR)
        {
            std::snprintf(
                line, sizeof(line), "%s var[%u]\n", name, (unsigned) s.u8);
        }
        else if (s.op == ALIA_COLOR_OP_IMMEDIATE)
        {
            std::snprintf(line, sizeof(line), "%s %.3f\n", name, (double) s.f);
        }
        else if (
            s.op == ALIA_COLOR_OP_GET || s.op == ALIA_COLOR_OP_SET
            || s.op == ALIA_COLOR_OP_CONTRAST)
        {
            std::snprintf(
                line, sizeof(line), "%s u8=%u\n", name, (unsigned) s.u8);
        }
        else
            std::snprintf(line, sizeof(line), "%s\n", name);
        int n = (int) std::strlen(line);
        int space = buf_size - 1 - written;
        if (n > space)
            n = space;
        std::memcpy(buf + written, line, (size_t) n);
        written += n;
    }
    buf[written] = '\0';
    return written;
}

} // extern "C"
