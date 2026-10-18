#include <alia/kernel/flow/dispatch.h>

#include <alia/abi/base/arena.h>
#include <alia/abi/kernel/effect.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/geometry.h>
#include <alia/abi/ui/styling.h>
#include <alia/context.h>
#include <alia/impl/base/arena.hpp>
#include <alia/impl/events.hpp>
#include <alia/ui/system/object.h>

#include <cstring>

namespace alia {

// TODO: Separate the kernel and UI aspects of this.
void
invoke_controller(ui_system& sys, event_traversal& events)
{
    alia_bump_allocator scratch;
    alia_bump_allocator_init(&scratch, &sys.kernel.scratch);

    alia_box clip_box = {{0, 0}, alia_vec2i_to_vec2f(sys.surface_size)};
    alia_box* clip_pointer = arena_new<alia_box>(scratch);
    *clip_pointer = clip_box;

    alia_geometry_context geometry = {
        .scale = sys.magnification * sys.dpi / 96.0f,
        .offset = {0, 0},
        .clip = {
            .box = clip_box,
            .pointer = clip_pointer,
            .id = 0,
        },
        .next_clip_id = 1,
        .z_base = ALIA_DEFAULT_Z_BASE,
    };

    alia_layout_context layout = {};
    layout.system = &sys.layout;
    if (events.event->type == ALIA_EVENT_REFRESH)
        layout.emission.next_ptr = &sys.layout.root.first_child;
    alia_bump_allocator_init(&layout.emission.arena, &sys.layout.node_arena);
    alia_bump_allocator_init(&layout.placement, &sys.layout.placement.arena);

    alia_stack_reset(&sys.kernel.stack);

    // TODO: Not this?
    alia_draw_context* draw = (events.event->type == ALIA_EVENT_DRAW)
                                ? as_draw_event(*events.event).context
                                : nullptr;

    alia_substrate_traversal substrate_traversal;
    alia_bump_allocator substrate_bump_allocator;
    alia_bump_allocator_init(
        &substrate_bump_allocator, &sys.kernel.substrate_discovery_arena);
    substrate_traversal_init(
        substrate_traversal,
        sys.kernel.substrate,
        &substrate_bump_allocator,
        sys.kernel.refresh_counter,
        events.event->type == ALIA_EVENT_REFRESH);

    ALIA_ASSERT(sys.styles.defaults && sys.styles.defaults_size > 0);
    size_t const actives_bytes = alia::align_up(sys.styles.defaults_size);
    void* active_styles
        = alia_arena_ptr(&scratch, alia_arena_alloc(&scratch, actives_bytes));
    std::memcpy(active_styles, sys.styles.defaults, sys.styles.defaults_size);

    alia_effect_log effects = {};

    alia_context ctx = {
        .kernel = &sys.kernel,
        .substrate = &substrate_traversal,
        .events = &events,
        .stack = &sys.kernel.stack,
        .scratch = &scratch,
        .tick_count = sys.kernel.tick_count,
        .effects = &effects,
        .system = &sys,
        .active_styles = active_styles,
        // No active font until something pushes one (see `alia_font_push`).
        // Using text before then is a programming error and asserts.
        .active_font = nullptr,
        // Resolve the default text color for the system palette.
        .text_color = alia_palette_color_resolve(
            &sys.palette, ALIA_PALETTE_COLOR(foundation.text.base, 0xff)),
        .geometry = &geometry,
        .input = &sys.input,
        .layout = &layout,
        .draw = draw,
        .palette = &sys.palette,
    };

    alia_substrate_begin_block(
        &ctx,
        &sys.kernel.substrate.root_anchor,
        &sys.kernel.substrate.root_block_spec);

    sys.controller.fn(sys.controller.user_data, &ctx);

    sys.kernel.substrate.root_block_spec = alia_substrate_end_block(&ctx);

    alia_run_effects(&ctx);

    if (events.event->type == ALIA_EVENT_REFRESH)
        *layout.emission.next_ptr = 0;
}

namespace {

void
route_event_(
    ui_system& sys, event_traversal& traversal, component_container* target)
{
    // In order to construct the path to the target, we start at the target
    // and follow the 'parent' pointers until we reach the root. We do this
    // via recursion so that the path can be constructed entirely on the
    // stack.
    // TODO: Construct the path in scratch space instead.
    if (target)
    {
        event_routing_path path_node;
        path_node.rest = traversal.path_to_target;
        path_node.node = target;
        traversal.path_to_target = &path_node;
        route_event_(sys, traversal, target->parent.get());
    }
    else
    {
        invoke_controller(sys, traversal);
    }
}

} // namespace

namespace detail {

// Set up the event traversal so that it will route the control flow to the
// given target. (And also invoke the traversal.)
//
// :target can be null, in which case the event will be routed through the
// entire component tree.
//
void
route_event(
    ui_system& sys, event_traversal& traversal, component_container* target)
{
    // printf("CODE: %08x\n", traversal.type_code);
    route_event_(sys, traversal, target);
}

} // namespace detail

} // namespace alia
