#ifndef ALIA_ABI_UI_REGIONS_H
#define ALIA_ABI_UI_REGIONS_H

#include <alia/abi/base/arena.h>
#include <alia/abi/base/geometry.h>
#include <alia/abi/kernel/routing.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/geometry/types.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;
typedef struct alia_ui_context alia_ui_context;

// flags for element participation in hit-test queries
typedef uint8_t alia_region_flags_t;

#define ALIA_REGION_FLAGS(X)                                                  \
    X(0x01, MOUSE, "accepts pointer hit-testing")                             \
    X(0x02, SCROLL_INPUT, "accepts scroll / wheel hit-testing")               \
    X(0x04, TOUCH_DRAG, "accepts touch-drag hit-testing")

#define ALIA_DEFINE_REGION_FLAG(code, name, description)                      \
    ALIA_REGION_##name = (code),
enum
{
    ALIA_REGION_FLAGS(ALIA_DEFINE_REGION_FLAG)
};
#undef ALIA_DEFINE_REGION_FLAG

// mutually exclusive roles for nodes in the region tree
typedef uint8_t alia_region_kind_t;

enum
{
    // a box representing an element's geometry
    ALIA_REGION_KIND_ELEMENT = 1,
    // clipping box
    ALIA_REGION_KIND_CLIP = 2,
    // translation
    ALIA_REGION_KIND_TRANSLATE = 3,
};

// a node in the region tree - Built during `ALIA_EVENT_REGIONS`.
typedef struct alia_region
{
    struct alia_region* parent;
    struct alia_region* next;
    struct alia_region* first_child;

    alia_region_kind_t kind;

    union
    {
        // ALIA_REGION_KIND_ELEMENT
        struct
        {
            alia_element_id id;
            // axis-aligned box, in content coordinates
            alia_box box;
            alia_z_index z;
            alia_region_flags_t flags;
        } element;

        // ALIA_REGION_KIND_CLIP
        // axis-aligned box, in content coordinates
        alia_box clip;

        // ALIA_REGION_KIND_TRANSLATE
        // translation, in content coordinates
        alia_vec2f translation;
    };
} alia_region;

// pass-local region tree build state - Present on the UI context only during
// `ALIA_EVENT_REGIONS`.
typedef struct alia_regions_context
{
    alia_bump_allocator arena;
    // node whose child list is being appended to
    alia_region* current;
    // append cursor for `current`'s children
    alia_region** next_child;
} alia_regions_context;

// element under a point together with its absolute Z
typedef struct alia_hit_target
{
    alia_element_id id;
    alia_z_index z;
} alia_hit_target;

typedef struct alia_mouse_hit_test_result
{
    alia_hit_target target;
    // element bounds in surface coordinates (for tooltips, overlays, etc.)
    alia_box region;
} alia_mouse_hit_test_result;

typedef struct alia_scroll_input_hit_test_result
{
    alia_hit_target target;
} alia_scroll_input_hit_test_result;

typedef struct alia_touch_gesture_hit_test_result
{
    // frontmost element under the point that accepts pointer interaction
    alia_hit_target pointer;
    // frontmost scroll-view (or similar) that can consume touch pan scrolling
    alia_hit_target scroll;
    // frontmost touch drag handle under the point (e.g. slider track,
    // scrollbar thumb) - When set, touch pan scrolling should not preempt
    // manipulation.
    alia_hit_target touch_drag;
} alia_touch_gesture_hit_test_result;

// Associate a hittable axis-aligned box with `id` in the current geometry
// nesting. `box` is in the current content coordinates. No-ops when the
// active pass is not a regions pass.
void
alia_element_box_region(
    alia_ui_context* ctx,
    alia_element_id id,
    alia_box const* box,
    alia_z_index z_plane,
    alia_region_flags_t flags);

// Clear and rebuild the system's region tree by emitting `ALIA_EVENT_REGIONS`.
void
alia_ui_rebuild_regions(alia_ui_system* ui);

bool
alia_ui_hit_test(
    alia_ui_system const* ui,
    float x,
    float y,
    alia_region_flags_t flags,
    alia_hit_target* out_target,
    alia_box* out_surface_box);

void
alia_ui_mouse_hit_test(
    alia_ui_system const* ui,
    float x,
    float y,
    alia_mouse_hit_test_result* out);

void
alia_ui_scroll_input_hit_test(
    alia_ui_system const* ui,
    float x,
    float y,
    alia_scroll_input_hit_test_result* out);

void
alia_ui_touch_gesture_hit_test(
    alia_ui_system const* ui,
    float x,
    float y,
    alia_touch_gesture_hit_test_result* out);

// Look up `id` in the region map and dispatch `SCROLL_INTO_VIEW` so scroll
// parents can bring its region on-screen. Returns false if `id` is not in
// the map.
bool
alia_ui_scroll_into_view(alia_ui_system* ui, alia_element_id id);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_REGIONS_H */
