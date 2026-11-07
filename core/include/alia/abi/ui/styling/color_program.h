#ifndef ALIA_ABI_UI_STYLING_COLOR_PROGRAM_H
#define ALIA_ABI_UI_STYLING_COLOR_PROGRAM_H

#include <alia/abi/base/arena.h>
#include <alia/abi/base/color.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/styling/environment.h>

// COLOR PROGRAMS
//
// Widget styles store compact programs that derive paint and label colors from
// the style environment at resolve time, rather than baking fixed sRGB. That
// keeps chrome live with palette and var changes without regenerating every
// style blob.
//
// The environment (`alia_style_environment`) supplies two loadable tables:
// palette colors (`LOAD_COLOR`) and style vars (`LOAD_VAR`). Seeds are also
// part of the environment but have no load opcode - generators bake
// seed-driven sizes as immediates or style fields instead.
//
// Evaluation is a postfix stack machine with a color stack and a float stack.
// The color top-of-stack (TOS) is the value under construction. Each color is
// OKLCH plus alpha (hue channel in radians). Ops move values between the
// stacks, edit channels, blend/composite colors, or adjust lightness for
// contrast.
//
// A program is a contiguous span of `alia_color_program_step` values. Styles
// hold a `steps` base pointer and relative `alia_color_program_ref` spans.
// Authors emit into an `alia_color_program_emitter` (usually scratch), then
// retarget `steps` to that region or `materialize` it into durable storage.
//
// Interaction is modeled as a 'rest' program plus optional continuations
// (`alia_responsive_color_program`). Continuations run with the rest result
// already on the color stack, so hover/active/disabled can be short deltas.
// Resolve APIs evaluate rest (and the continuation for a widget status)
// against the environment and return opaque sRGBA for drawing.

ALIA_EXTERN_C_BEGIN

// maximum color stack depth
#define ALIA_COLOR_PROGRAM_STACK_DEPTH 8

// maximum float stack depth
#define ALIA_COLOR_FLOAT_STACK_DEPTH 8

// placement of a color solid relative to the page (or absolute L)
typedef uint32_t alia_color_tone_t;

enum
{
    // quiet solid near the page (SURFACE + match_step toward TEXT)
    ALIA_COLOR_TONE_MATCH = 0,
    // absolute dark tint of the color (DARK_L)
    ALIA_COLOR_TONE_DARK = 1,
    // absolute light tint of the color (LIGHT_L)
    ALIA_COLOR_TONE_LIGHT = 2,
    // punchy solid opposite the page (SURFACE + inverse_step toward TEXT)
    ALIA_COLOR_TONE_INVERSE = 3
};

// TOS channel for GET / SET (stored in step.u8)
typedef enum alia_color_channel
{
    ALIA_COLOR_CHANNEL_L = 0,
    ALIA_COLOR_CHANNEL_C = 1,
    ALIA_COLOR_CHANNEL_H = 2,
    ALIA_COLOR_CHANNEL_A = 3
} alia_color_channel;

// lightness search direction for CONTRAST
typedef enum alia_color_l_direction
{
    // prefer white vs black ink by relative luminance vs against
    ALIA_COLOR_L_AUTO = 0,
    // toward L = 1
    ALIA_COLOR_L_LIGHT = 1,
    // toward L = 0
    ALIA_COLOR_L_DARK = 2,
    // toward TEXT.l
    ALIA_COLOR_L_STRONG = 3,
    // toward SURFACE.l
    ALIA_COLOR_L_WEAK = 4
} alia_color_l_direction;

// how strictly CONTRAST honors the direction
typedef enum alia_color_contrast_constraint
{
    // try the preferred side first, then flip if the target cannot be met
    ALIA_COLOR_CONTRAST_PREFER = 0,
    // search only the preferred side (best-effort clamp on that side)
    ALIA_COLOR_CONTRAST_REQUIRE = 1
} alia_color_contrast_constraint;

// against-color source for CONTRAST (packed into step.u8)
typedef enum alia_color_contrast_against
{
    // pop against color from the color stack
    ALIA_COLOR_CONTRAST_AGAINST_STACK = 0,
    // synthetic white (L=1, C=0)
    ALIA_COLOR_CONTRAST_AGAINST_WHITE = 1,
    // synthetic black (L=0, C=0)
    ALIA_COLOR_CONTRAST_AGAINST_BLACK = 2
} alia_color_contrast_against;

// Pack CONTRAST options into step.u8.
static inline uint8_t
alia_color_contrast_pack(
    alia_color_l_direction direction,
    alia_color_contrast_constraint constraint,
    alia_color_contrast_against against)
{
    return (uint8_t) (((unsigned) direction & 0x7u)
                      | (((unsigned) constraint & 0x1u) << 3)
                      | (((unsigned) against & 0x3u) << 4));
}

static inline alia_color_l_direction
alia_color_contrast_direction(uint8_t packed)
{
    return (alia_color_l_direction) (packed & 0x7u);
}

static inline alia_color_contrast_constraint
alia_color_contrast_constraint_of(uint8_t packed)
{
    return (alia_color_contrast_constraint) ((packed >> 3) & 0x1u);
}

static inline alia_color_contrast_against
alia_color_contrast_against_of(uint8_t packed)
{
    return (alia_color_contrast_against) ((packed >> 4) & 0x3u);
}

// program span relative to a style's `steps` pointer
typedef struct alia_color_program_ref
{
    uint16_t offset;
    uint8_t count;
} alia_color_program_ref;

// Return an empty program ref (no steps).
static inline alia_color_program_ref
alia_color_program_ref_none(void)
{
    alia_color_program_ref ref;
    ref.offset = 0;
    ref.count = 0;
    return ref;
}

// color ops - `X(NAME, "brief description")`
// clang-format off
#define ALIA_COLOR_OPS(X)                                                     \
    X(LOAD_COLOR, "colors[u8] -> color stack")                                \
    X(LOAD_VAR, "vars[u8] -> float stack")                                    \
    X(IMMEDIATE, "literal f -> float stack")                                  \
    X(DUP_COLOR, "duplicate TOS color")                                       \
    X(DROP_COLOR, "drop TOS color")                                           \
    X(SWAP_COLOR, "swap the top two colors")                                  \
    X(DUP_FLOAT, "duplicate TOS float")                                       \
    X(DROP_FLOAT, "drop TOS float")                                           \
    X(SWAP_FLOAT, "swap the top two floats")                                  \
    X(ADD, "float: (b + a) for [b, a]")                                       \
    X(SUB, "float: (b - a) for [b, a]")                                       \
    X(MUL, "float: (b * a) for [b, a]")                                       \
    X(ABS, "float: |a| for [a]")                                              \
    X(MIN, "float: min(b, a) for [b, a]")                                     \
    X(LERP, "float: a+(b-a)*t for [a, b, t]")                                 \
    X(SNAP, "float: snap h to accent lattice for [h, accent, step]")          \
    X(OVERLAY, "float: coverage alpha for [fg_l, bg_l, step]")                \
    X(GET, "TOS channel (u8) -> float stack")                                 \
    X(SET, "float -> TOS channel (u8)")                                       \
    X(BLEND, "lerp TOS toward pop color by pop float (alpha unchanged)")      \
    X(OVER, "composite TOS over next color (linear)")                         \
    X(CONTRAST, "adjust TOS.l for contrast (see u8); pop ratio")
// clang-format on

enum alia_color_op
{
#define ALIA_COLOR_OP_X(NAME, desc) ALIA_COLOR_OP_##NAME,
    ALIA_COLOR_OPS(ALIA_COLOR_OP_X)
#undef ALIA_COLOR_OP_X
    ALIA_COLOR_OP_COUNT
};

// Return a short description of `op`, or null if unknown.
char const*
alia_color_op_description(uint8_t op);

// one program step - `op` plus a byte operand and a float operand. Unused
// fields are ignored. Layout is 8 bytes (aligned to `ALIA_MIN_ALIGN`).
typedef struct alia_color_program_step
{
    uint8_t op;
    // byte operand - Color/float index, channel, or packed CONTRAST flags.
    uint8_t u8;
    // float operand - Literal for IMMEDIATE (unused otherwise).
    float f;
} alia_color_program_step;

// emission cursor into a bump arena - Steps for one style form a contiguous
// region. Refs are relative to that region's base (`base`).
typedef struct alia_color_program_emitter
{
    alia_bump_allocator* bump;
    // first step of this style region (null until the first emit)
    alia_color_program_step* base;
    uint16_t count;
} alia_color_program_emitter;

// rest program plus interaction continuations - Continuations run with the
// rest result seeded as the initial TOS. A zero `count` means no change.
typedef struct alia_responsive_color_program
{
    alia_color_program_ref rest;
    alia_color_program_ref hovered;
    alia_color_program_ref active;
    alia_color_program_ref disabled;
} alia_responsive_color_program;

// fill/border responsive programs for a box
typedef struct alia_responsive_box_paint_program
{
    alia_responsive_color_program fill;
    alia_responsive_color_program fill_end;
    alia_responsive_color_program border;
    alia_responsive_color_program border_end;
    bool gradient;
    // shape-local degrees
    float angle;
    // outline width, logical px
    float border_width;
} alia_responsive_box_paint_program;

struct alia_style_environment;
struct alia_kernel_context;
struct alia_general_allocator;

// Begin a style region at the current bump cursor.
alia_color_program_emitter
alia_color_program_emitter_begin(alia_bump_allocator* bump);

// Return the step base for the current region, or null if nothing has been
// emitted yet.
static inline alia_color_program_step*
alia_color_program_emitter_steps(alia_color_program_emitter const* emitter)
{
    return emitter->base;
}

// Return how many steps have been emitted in the current region.
static inline uint16_t
alia_color_program_emitter_count(alia_color_program_emitter const* emitter)
{
    return emitter->count;
}

// Append `step` and return 0 on success.
int
alia_color_program_emit_step(
    alia_color_program_emitter* emitter, alia_color_program_step step);

// Append a span of steps copied from `src` / `ref` (relative to `src`).
alia_color_program_ref
alia_color_program_emit_copy(
    alia_color_program_emitter* emitter,
    alia_color_program_step const* src,
    alia_color_program_ref ref);

// Mark the end of a program started at `start` (region-relative) and return
// the relative ref.
alia_color_program_ref
alia_color_program_end_ref(
    alia_color_program_emitter const* emitter, uint16_t start);

// materialize flags
enum
{
    // allocate via `allocator` and copy the emitter region
    ALIA_COLOR_PROGRAM_MATERIALIZE_OWNED = 0,
    // fold into the current substrate block (`ctx` required) - Size must stay
    // stable across visits.
    ALIA_COLOR_PROGRAM_MATERIALIZE_FOLD = 1u << 0,
};

// Publish the emitter's region into durable storage and return the new base.
// Scratch contents are left unchanged. `allocator` is required for OWNED.
alia_color_program_step*
alia_color_program_materialize(
    struct alia_kernel_context* ctx,
    alia_color_program_emitter const* emitter,
    struct alia_general_allocator* allocator,
    uint32_t flags);

// STEP CONSTRUCTORS
//
// Hue immediates for SET H are stored in radians (matching `alia_oklch.h`).

// default lattice step for hue snap (degrees)
#define ALIA_COLOR_PROGRAM_HARMONIZE_STEP_DEG 15.f

// Convert hue degrees to radians for storage in hue immediates.
static inline float
alia_color_program_deg_to_rad(float degrees)
{
    return degrees * 0.017453292519943295769f;
}

static inline alia_color_program_step
alia_color_step_load_color(alia_style_color_slot color_index)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_LOAD_COLOR;
    s.u8 = color_index;
    return s;
}

static inline alia_color_program_step
alia_color_step_load_var(uint8_t var_index)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_LOAD_VAR;
    s.u8 = var_index;
    return s;
}

static inline alia_color_program_step
alia_color_step_immediate(float value)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_IMMEDIATE;
    s.f = value;
    return s;
}

static inline alia_color_program_step
alia_color_step_dup_color(void)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_DUP_COLOR;
    return s;
}

static inline alia_color_program_step
alia_color_step_drop_color(void)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_DROP_COLOR;
    return s;
}

static inline alia_color_program_step
alia_color_step_swap_color(void)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_SWAP_COLOR;
    return s;
}

static inline alia_color_program_step
alia_color_step_dup_float(void)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_DUP_FLOAT;
    return s;
}

static inline alia_color_program_step
alia_color_step_drop_float(void)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_DROP_FLOAT;
    return s;
}

static inline alia_color_program_step
alia_color_step_swap_float(void)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_SWAP_FLOAT;
    return s;
}

static inline alia_color_program_step
alia_color_step_add(void)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_ADD;
    return s;
}

static inline alia_color_program_step
alia_color_step_sub(void)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_SUB;
    return s;
}

static inline alia_color_program_step
alia_color_step_mul(void)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_MUL;
    return s;
}

static inline alia_color_program_step
alia_color_step_abs(void)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_ABS;
    return s;
}

static inline alia_color_program_step
alia_color_step_min(void)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_MIN;
    return s;
}

static inline alia_color_program_step
alia_color_step_lerp(void)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_LERP;
    return s;
}

static inline alia_color_program_step
alia_color_step_snap(void)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_SNAP;
    return s;
}

static inline alia_color_program_step
alia_color_step_overlay(void)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_OVERLAY;
    return s;
}

static inline alia_color_program_step
alia_color_step_get(alia_color_channel channel)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_GET;
    s.u8 = (uint8_t) channel;
    return s;
}

static inline alia_color_program_step
alia_color_step_set(alia_color_channel channel)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_SET;
    s.u8 = (uint8_t) channel;
    return s;
}

static inline alia_color_program_step
alia_color_step_blend(void)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_BLEND;
    return s;
}

static inline alia_color_program_step
alia_color_step_over(void)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_OVER;
    return s;
}

// CONTRAST against a popped stack color (prefer, auto).
static inline alia_color_program_step
alia_color_step_contrast(void)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_CONTRAST;
    s.u8 = alia_color_contrast_pack(
        ALIA_COLOR_L_AUTO,
        ALIA_COLOR_CONTRAST_PREFER,
        ALIA_COLOR_CONTRAST_AGAINST_STACK);
    return s;
}

// CONTRAST with explicit direction and constraint against stack color.
static inline alia_color_program_step
alia_color_step_contrast_ex(
    alia_color_l_direction direction,
    alia_color_contrast_constraint constraint)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_CONTRAST;
    s.u8 = alia_color_contrast_pack(
        direction, constraint, ALIA_COLOR_CONTRAST_AGAINST_STACK);
    return s;
}

// CONTRAST against synthetic white (L=1).
static inline alia_color_program_step
alia_color_step_contrast_against_white(
    alia_color_l_direction direction,
    alia_color_contrast_constraint constraint)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_CONTRAST;
    s.u8 = alia_color_contrast_pack(
        direction, constraint, ALIA_COLOR_CONTRAST_AGAINST_WHITE);
    return s;
}

// CONTRAST against synthetic black (L=0).
static inline alia_color_program_step
alia_color_step_contrast_against_black(
    alia_color_l_direction direction,
    alia_color_contrast_constraint constraint)
{
    alia_color_program_step s = {};
    s.op = (uint8_t) ALIA_COLOR_OP_CONTRAST;
    s.u8 = alia_color_contrast_pack(
        direction, constraint, ALIA_COLOR_CONTRAST_AGAINST_BLACK);
    return s;
}

// Format a program span into `buf` for introspection. Return the number of
// bytes that would have been written (like `snprintf`).
int
alia_color_program_snprint(
    char* buf,
    int buf_size,
    alia_color_program_step const* steps,
    alia_color_program_ref ref);

// Return a solid box paint program. End stops copy their starts.
static inline alia_responsive_box_paint_program
alia_responsive_box_paint_program_solid(
    alia_responsive_color_program fill,
    alia_responsive_color_program border,
    float border_width)
{
    alia_responsive_box_paint_program paint;
    paint.fill = fill;
    paint.fill_end = fill;
    paint.border = border;
    paint.border_end = border;
    paint.gradient = false;
    paint.angle = 0.f;
    paint.border_width = border_width;
    return paint;
}

// Return a gradient box paint program. `angle` is shape-local degrees.
static inline alia_responsive_box_paint_program
alia_responsive_box_paint_program_gradient(
    alia_responsive_color_program fill,
    alia_responsive_color_program fill_end,
    alia_responsive_color_program border,
    alia_responsive_color_program border_end,
    float angle,
    float border_width)
{
    alia_responsive_box_paint_program paint;
    paint.fill = fill;
    paint.fill_end = fill_end;
    paint.border = border;
    paint.border_end = border_end;
    paint.gradient = true;
    paint.angle = angle;
    paint.border_width = border_width;
    return paint;
}

// Resolve `rest` then optional continuation for `status` against `steps`.
alia_srgba8
alia_responsive_color_program_resolve(
    struct alia_style_environment const* env,
    alia_color_program_step const* steps,
    alia_responsive_color_program const* color,
    uint8_t status);

// Resolve a single program span (no continuation).
alia_srgba8
alia_color_program_resolve(
    struct alia_style_environment const* env,
    alia_color_program_step const* steps,
    alia_color_program_ref ref);

ALIA_EXTERN_C_END

#endif // ALIA_ABI_UI_STYLING_COLOR_PROGRAM_H
