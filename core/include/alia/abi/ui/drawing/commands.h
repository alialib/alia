#ifndef ALIA_ABI_UI_DRAWING_COMMANDS_H
#define ALIA_ABI_UI_DRAWING_COMMANDS_H

#include <alia/abi/base/arena.h>
#include <alia/abi/base/geometry.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/geometry/api.h>

ALIA_EXTERN_C_BEGIN

// IDs used when recording and bucketing draw commands
typedef uint16_t alia_draw_material_id;
typedef uint16_t alia_draw_target_id;
typedef uint16_t alia_frame_id;

enum
{
    // the base UI surface (i.e., the window)
    ALIA_DRAW_TARGET_PRIMARY = 0,
};

// no parent frame
#define ALIA_FRAME_ID_NONE ((alia_frame_id) 0xffff)

// `alia_draw_id` is a retained draw identity value (e.g. for maps).
//
// For components that animate, the following conventions are used:
// - `[0, ALIA_DRAW_ID_ANIMATION_SUB_BASE)` is reserved for component-authored
//   draw IDs.
// - Animation utilities occupy the band above that . A bitpack field at bit
//   offset `n` may use the `ALIA_DRAW_ID_ANIMATION_SUB_STRIDE` indices
//   starting at `ALIA_DRAW_ID_ANIMATION_SUB_BASE + n *
//   ALIA_DRAW_ID_ANIMATION_SUB_STRIDE`.
typedef struct alia_draw_id
{
    alia_instance_id id;
    uint32_t sub_id;
} alia_draw_id;

#define ALIA_DRAW_ID_NONE                                                     \
    ALIA_BRACED_INIT(alia_draw_id, ALIA_INSTANCE_ID_NONE, 0)

static inline alia_draw_id
alia_draw_id_make(alia_instance_id id, uint32_t sub_id)
{
    return ALIA_BRACED_INIT(alia_draw_id, id, sub_id);
}

static inline bool
alia_draw_id_is_valid(alia_draw_id id)
{
    return alia_instance_id_is_valid(id.id);
}

static inline bool
alia_draw_id_equal(alia_draw_id a, alia_draw_id b)
{
    return alia_instance_id_equal(a.id, b.id) && a.sub_id == b.sub_id;
}

enum
{
    ALIA_DRAW_ID_ANIMATION_SUB_BASE = 1 << 16,
    ALIA_DRAW_ID_ANIMATION_SUB_STRIDE = 1 << 10,
};

// Compute the first draw sub-index owned by an animation bitpack field at
// `bit_offset`.
static inline uint32_t
alia_draw_id_animation_sub_base(uint8_t bit_offset)
{
    return (uint32_t) ALIA_DRAW_ID_ANIMATION_SUB_BASE
         + (uint32_t) bit_offset
               * (uint32_t) ALIA_DRAW_ID_ANIMATION_SUB_STRIDE;
}

// a translation frame in the draw pass - Commands store boxes in the local
// space of their frame. `absolute` is the cumulative translation to surface
// (or current draw-target) space for packers.
typedef struct alia_draw_frame
{
    alia_frame_id parent;
    alia_vec2f translation;
    alia_vec2f absolute;
} alia_draw_frame;

typedef struct alia_draw_command
{
    struct alia_draw_command* next;
    // retained instance identity for diffing
    alia_instance_id id;
    // draw sub-index within the instance
    uint32_t sub_id;
    // coordinate frame for this command's geometry
    alia_frame_id frame;
} alia_draw_command;

static inline alia_frame_id
alia_draw_command_frame(alia_draw_command const* cmd)
{
    return cmd->frame;
}

static inline alia_draw_id
alia_draw_command_id(alia_draw_command const* cmd)
{
    return alia_draw_id_make(cmd->id, cmd->sub_id);
}

typedef struct alia_draw_bucket
{
    alia_box* clip_rect;
    alia_draw_command* head;
    alia_draw_command* tail;
    uint32_t count;
    // TODO: Add generalized, material-specific summary info.
    uint32_t instance_count;
} alia_draw_bucket;

typedef struct alia_draw_bucket_table alia_draw_bucket_table;
typedef struct alia_draw_frame_table alia_draw_frame_table;

typedef struct alia_draw_context
{
    alia_draw_bucket_table* buckets;
    alia_draw_frame_table* frames;
    alia_bump_allocator arena;
    // active draw target
    alia_draw_target_id target_id;
    // frame whose local space new commands use
    alia_frame_id current_frame;
} alia_draw_context;

// state that is available to materials while a draw pass executes
typedef struct alia_draw_execution_context
{
    // the frames recorded during the draw pass, indexed by `alia_frame_id`
    alia_draw_frame const* frames;
    uint16_t frame_count;
} alia_draw_execution_context;

// Resolve a local box into surface (or draw-target) space via its frame.
static inline alia_box
alia_draw_resolve_box(
    alia_draw_execution_context const* exec,
    alia_frame_id frame,
    alia_box local)
{
    ALIA_ASSERT(exec && frame < exec->frame_count);
    return alia_box_translate(local, exec->frames[frame].absolute);
}

static inline alia_box
alia_draw_resolve_command_box(
    alia_draw_execution_context const* exec,
    alia_draw_command const* cmd,
    alia_box local)
{
    return alia_draw_resolve_box(exec, alia_draw_command_frame(cmd), local);
}

// Allocate a draw command with `ALIA_MIN_ALIGN` alignment.
alia_draw_command*
alia_draw_command_alloc(
    alia_ui_context* ctx,
    alia_draw_id id,
    alia_z_index z_index,
    alia_draw_material_id material_id,
    size_t size);

// Push a translation frame for the draw pass. Do nothing when not drawing.
void
alia_draw_push_frame(alia_ui_context* ctx, alia_vec2f translation);

// Pop the current draw translation frame. Do nothing when not drawing.
void
alia_draw_pop_frame(alia_ui_context* ctx);

// Reset to a new root frame (absolute origin). This is used when entering an
// offscreen draw target. Return the previous frame ID for later restoration.
alia_frame_id
alia_draw_begin_root_frame(alia_ui_context* ctx);

// Restore `frame` as current after `alia_draw_begin_root_frame`.
void
alia_draw_restore_frame(alia_ui_context* ctx, alia_frame_id frame);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_DRAWING_COMMANDS_H */
