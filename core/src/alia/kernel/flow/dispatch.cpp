#include <alia/kernel/flow/dispatch.h>

#include <alia/abi/base/arena.h>
#include <alia/abi/base/memory.h>
#include <alia/abi/kernel/effect.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/geometry/api.h>
#include <alia/abi/ui/styling/catalog.h>
#include <alia/impl/base/arena.hpp>
#include <alia/impl/context.hpp>
#include <alia/impl/events.hpp>
#include <alia/ui/system/object.h>

#include <cstring>

namespace alia {

// TODO: Separate the kernel and UI aspects of this.
void
invoke_controller(ui_system& sys, event_traversal& events)
{
    bool const refresh = events.event->type == ALIA_EVENT_REFRESH;
    alia_memory_system* memory = &sys.kernel.memory;

    alia_layout_context layout = {};
    layout.system = &sys.layout;
    if (refresh)
    {
        layout.emission.next_ptr = &sys.layout.root.first_child;
        layout.emission.arena
            = alia_memory_open_bump(memory, sys.layout.node_pool);
    }
    else
    {
        layout.placement
            = alia_memory_open_bump(memory, sys.layout.placement_pool);
    }

    alia_stack pass_stack
        = alia_memory_open_stack(memory, sys.kernel.stack_pool);

    // TODO: Not this?
    alia_draw_context* draw = (events.event->type == ALIA_EVENT_DRAW)
                                ? as_draw_event(*events.event).context
                                : nullptr;

    alia_substrate_traversal substrate_traversal;
    alia_bump_allocator substrate_bump_allocator
        = alia_memory_open_bump(memory, sys.kernel.substrate_discovery_pool);
    substrate_traversal_init(
        substrate_traversal,
        sys.kernel.substrate,
        &substrate_bump_allocator,
        sys.kernel.refresh_counter,
        refresh);

    alia_ui_context ctx = {};
    ctx.kernel.object = &sys.kernel;
    ctx.kernel.substrate = &substrate_traversal;
    ctx.kernel.events = &events;
    ctx.kernel.stack = &pass_stack;
    ctx.kernel.tick_count = sys.kernel.tick_count;
    ctx.kernel.scratch
        = alia_memory_open_bump(memory, sys.kernel.scratch_pool);

    alia_box clip_box = {{0, 0}, alia_vec2i_to_vec2f(sys.surface_size)};
    alia_box* clip_pointer = arena_new<alia_box>(ctx.kernel.scratch);
    *clip_pointer = clip_box;
    ctx.geometry = {
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

    ALIA_ASSERT(sys.styles.defaults && sys.styles.defaults_size > 0);
    size_t const actives_bytes = alia::align_up(sys.styles.defaults_size);
    void* active_styles = alia_arena_ptr(
        &ctx.kernel.scratch,
        alia_arena_alloc(&ctx.kernel.scratch, actives_bytes));
    std::memcpy(active_styles, sys.styles.defaults, sys.styles.defaults_size);

    ctx.system = &sys;
    ctx.style.active_styles = active_styles;
    // No active font until something pushes one (see `alia_font_push`).
    // Using text before then is a programming error and asserts.
    ctx.style.text.font = nullptr;
    // Resolve the default text color for the system palette.
    ctx.style.text.color = alia_palette_color_resolve(
        alia_palette_colors(&sys.palette),
        ALIA_PALETTE_COLOR(foundation.text.base, 0xff));
    alia_style_apply_seeds(&ctx.style, &sys.styles.seeds);
    ctx.input = &sys.input;
    ctx.layout = &layout;
    ctx.draw = draw;
    ctx.palette = &sys.palette;
    ctx.id.metrics = alia_id_view_make_u64(sys.metrics_revision);
    ctx.id.paint = alia_id_view_make_u64(sys.paint_revision);

    alia_substrate_begin_block(
        alia_ctx_kernel(&ctx),
        &sys.kernel.substrate.root_anchor,
        &sys.kernel.substrate.root_block_spec);

    sys.controller.fn(sys.controller.user_data, &ctx);

    sys.kernel.substrate.root_block_spec
        = alia_substrate_end_block(alia_ctx_kernel(&ctx));

    alia_run_effects(alia_ctx_kernel(&ctx));

    if (refresh)
    {
        *layout.emission.next_ptr = 0;
        alia_memory_commit_bump(
            memory, sys.layout.node_pool, &layout.emission.arena);
        alia_memory_trim(
            memory, sys.layout.node_pool, ALIA_MEMORY_RETAIN_USED);
    }
    else
    {
        alia_memory_discard_bump(memory, sys.layout.placement_pool);
    }

    alia_memory_commit_bump(
        memory, sys.kernel.scratch_pool, &ctx.kernel.scratch);
    alia_memory_trim(memory, sys.kernel.scratch_pool, ALIA_MEMORY_RETAIN_PEAK);

    alia_memory_commit_bump(
        memory,
        sys.kernel.substrate_discovery_pool,
        &substrate_bump_allocator);
    alia_memory_trim(
        memory, sys.kernel.substrate_discovery_pool, ALIA_MEMORY_RETAIN_PEAK);

    alia_memory_commit_stack(memory, sys.kernel.stack_pool, &pass_stack);
    alia_memory_trim(memory, sys.kernel.stack_pool, ALIA_MEMORY_RETAIN_PEAK);
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
